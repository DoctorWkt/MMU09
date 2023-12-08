// init: The initial user-level program.
// This code from xv6.

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

void cprintf(char *fmt, ...);

char *argv[] = { "/bin/sh", 0 };

int main(void) {
  int pid, wpid;
  int wstatus;

#if 0
  // The kernel, at present, opens fds 0, 1 and 2
  if (open("/dev/console", O_RDWR) < 0) {
    mknod("/dev/console", 1, 1);
    open("/dev/console", O_RDWR);
  }
  dup(0);			// stdout
  dup(0);			// stderr
#endif

  while (1) {
    cprintf("init: starting %s\n", argv[0]);
    pid = fork();
    if (pid < 0) {
      cprintf("init: fork failed\n");
      exit(1);
    }

    if (pid == 0) {
      exec(argv[0], argv);
      cprintf("init: exec %s failed\n", argv[0]);
      exit(1);
    }

    while ((wpid = wait(&wstatus)) >= 0 && wpid != pid)
      cprintf("zombie pid %x exited\n", wpid);
  }
  return (0);
}
