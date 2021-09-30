#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int to_parent[2];
  int to_child[2];

  pipe(to_parent);
  pipe(to_child);

  if(pipe(to_parent) == -1 || pipe(to_child) == -1){
    fprintf(2, "error occured when creating pipe");
    exit(-1);
  }

  int pid = fork();

  if(pid < 0){
    fprintf(2, "fork unsuccessful");
    exit(-1);
  } else if(pid == 0){
    char received;

    // Close unused pipe ends in child process
    close(to_child[1]);
    close(to_parent[0]);

    read(to_child[0], &received, 1);
    printf("%d: received ping\n", getpid());
    write(to_parent[1], "a", 1);
    close(to_parent[1]);

  } else {
    char received;

    // Close unused pipe ends in parent process
    close(to_child[0]);
    close(to_parent[1]);

    write(to_child[1], "a", 1);
    read(to_parent[0], &received, 1);
    printf("%d: received pong\n", getpid());
    close(to_parent[0]);
  }

  exit(0);
}

