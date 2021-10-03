#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "stddef.h"

int
main(int argc, char *argv[])
{
  if(argc < 3){
    fprintf(2, "usage: xargs <command> <args>\n");
    exit(-1);
  }
  
  // Alocate buffer with size of MAXARG 
  char *buf = (char *)malloc(MAXARG*sizeof(char));
  char c;
  int i = 0;
  
  // Read characters until newline appears
  while(read(0, &c, sizeof(char)) > 0){
    if(c  == '\n'){
      // Create exec arguments vector and add command, args and buffer to it 
      char *exec_argv[4];
      exec_argv[0] = argv[1];
      exec_argv[1] = argv[2];
      exec_argv[2] = buf;
      exec_argv[3] = 0;

      int pid = fork();
      
      if(pid < 0){
	fprintf(2, "fork failed");
	exit(-1);
      } else if(pid == 0){
	exec(argv[1], exec_argv);
	fprintf(2, "exec failed\n");
	exit(-1);
      } else {
	// Wait for child process to execute command
	wait(0);
      }
    
      // Free and clear buffer for the next arguments
      i = 0;
      memset(buf, 0, sizeof(buf));
      free(buf);
      buf = NULL;
      buf = (char *)malloc(MAXARG*sizeof(char));

    } else {
      buf[i] = c;
      i++;
    }
  }
  exit(0);
}
