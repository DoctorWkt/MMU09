#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void cprintf(char *fmt, ...);

int main(int argc, char *argv[]) {
  int pid;
  int wstatus;
  int pipefd[2];
  char buf[512];

  char *mesg= "Hello world!!\n";

  cprintf("In the parent, about to open a pipe\n");
  wstatus= pipe(pipefd);
  cprintf("pipe returned %d\n", wstatus);
  if (wstatus==-1) exit(1);
  cprintf("pipe fds are %d and %d\n", pipefd[0], pipefd[1]);

  cprintf("In the parent, about to fork()\n");
  pid= fork();

  switch(pid) {
    case 0:
	// Close the pipe write end
	close(pipefd[1]);
	// Read from the pipe
	wstatus = read(pipefd[0], buf, 512);
	cprintf("In the child, pipe read returned %d\n", wstatus);
	if (wstatus != -1) cprintf("pipe contents: %s\n", buf);
	cprintf("In the child, about to exit(0)\n");
	exit(0);

    case -1:
	cprintf("fork failed!\n");
	exit(1);

    default:
	// Close the pipe read end
	close(pipefd[0]);
	// Write stuff down the pipe
	cprintf("In the parent, about to write down the pipe\n");
	write(pipefd[1], mesg, strlen(mesg)+1);
	cprintf("In the parent, about to wait()\n");
	pid= wait(&wstatus);
	cprintf("wait returned pid %d status %x\n", pid, wstatus);
  }

  return(0);
}
