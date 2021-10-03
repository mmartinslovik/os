#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  char *p;

  // Find first character after slash.
    for (p=path+strlen(path); p >= path && *p != '/'; p--)
      ;
    p++;
    // Return name
    return p;
}

void
find(char *path, char *filename)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    exit(-1);
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    exit(-1);
  }

  switch(st.type){
  case T_FILE:
      p = fmtname(path); 
      // If filename is found print it 
      if(strcmp(p, filename) == 0){
	printf("%s\n", path);
      }
      break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    // Use buf to increase path
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
     // Do not recurse into directory "." and  ".." 
      if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
	continue;
    
      // Append directory name to path
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      // Recurse into subdirectory
      find(buf, filename);
    }
    break;
  }
  close(fd);
}


int 
main(int argc, char * argv[])
{
  if(argc < 3){
    fprintf(2, "usage: find <path> <filename>\n");
    exit(-1);
  }

  find(argv[1], argv[2]);

  exit(0);
}

