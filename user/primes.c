#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
create_prime(int parent_fd[2])
{
  int p;
  close(parent_fd[1]);

  // Read and print prime number
  if(read(parent_fd[0], &p, sizeof(int)) > 0){
      printf("prime %d\n", p);
  }

  int n;

  // If pipe is empty do not create new process
  if(read(parent_fd[0], &n, sizeof(int)) != 0){
    int child_fd[2];

    if(pipe(child_fd) < 0){
      fprintf(2, "error occured when creating pipe\n");
      exit(-1);
    }

    int pid = fork();

    if(pid < 0){
      fprintf(2, "creating fork unsuccessful\n");
      exit(-1);
    } else if(pid == 0){
      create_prime(child_fd);
      exit(0);
    } else {
      close(child_fd[0]);

      while(1){
	if(n%p != 0){
	  // Write to the neighbor on right
	  write(child_fd[1], &n, sizeof(int));
	}
	// Read from neighbor on left
	if(read(parent_fd[0], &n, sizeof(int)) == 0){
	  break;
	}
      }
      // Close unused pipe ends and wait for child processes to finish
      close(child_fd[1]);
      close(parent_fd[0]);
      wait(0);
      exit(0);
    }
  }
  exit(0);
}

int
main(int argc, char *argv[])
{
  int parent_fd[2];

  if(pipe(parent_fd) < 0){
    fprintf(2, "error occured when creating pipe\n");
  }

  int pid = fork();

  if(pid < 0){
    fprintf(2, "creating fork unsuccessful\n");
    exit(-1);
  } else if(pid == 0){
    // Create new process
    create_prime(parent_fd);
    exit(0);
  } else {
    int i;
    close(parent_fd[0]);
    
    // Iterate numbers into pipe
    for(i = 2; i < 36; i++) {
      // Write to the neighbor on right
      write(parent_fd[1], &i, sizeof(int));
  } 

  close(parent_fd[1]);
  // Wait until all child processes end 
  wait(0);
  exit(0);
  }
}
