// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#include "limits.h"

#define NBUCKET 13

struct {
  struct spinlock lock;
  struct spinlock bucket_lock[NBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
} bcache;


// Set ticks to the buffer
void
assign_ticks(struct buf *b)
{
  acquire(&tickslock);
  b->ticks = ticks;
  release(&tickslock);
}


// Move current bucket to destination bucket
void
move_bucket(struct buf *b, int destination_bucket)
{
  b->next->prev = b->prev;
  b->prev->next = b->next;
  b->next = bcache.head[destination_bucket].next;
  b->prev = &bcache.head[destination_bucket];
  bcache.head[destination_bucket].next->prev = b;
  bcache.head[destination_bucket].next = b;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.bucket_lock[i], "bcache.bucket");
    // Create linked lists of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->blockno = 0;
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int bucket = blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket]);

  // Is the block already cached?
  for(b = bcache.head[bucket].next; b != &bcache.head[bucket]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_lock[bucket]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  acquire(&bcache.lock);
  //struct buf *lru;
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.buf; b != bcache.buf + NBUF; b++){
    int candidate_bucket = b->blockno % NBUCKET;
    if(candidate_bucket != bucket){
      acquire(&bcache.bucket_lock[candidate_bucket]);
    }
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      if(candidate_bucket != bucket){
	move_bucket(b, bucket);
	release(&bcache.bucket_lock[candidate_bucket]);
      }
      release(&bcache.lock);
      release(&bcache.bucket_lock[bucket]);
      acquiresleep(&b->lock);
      return b;
    }
    if(candidate_bucket != bucket){
      release(&bcache.bucket_lock[candidate_bucket]);
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucket = b->blockno % NBUCKET;
  assign_ticks(b);
  acquire(&bcache.bucket_lock[bucket]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[bucket].next;
    b->prev = &bcache.head[bucket];
    bcache.head[bucket].next->prev = b;
    bcache.head[bucket].next = b;
  } 
  release(&bcache.bucket_lock[bucket]);
}

void
bpin(struct buf *b)
{
  int bucket = b->blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket]);
  b->refcnt++;
  release(&bcache.bucket_lock[bucket]);
}

void
bunpin(struct buf *b)
{
  int bucket = b->blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket]);
  b->refcnt--;
  release(&bcache.bucket_lock[bucket]);
}


