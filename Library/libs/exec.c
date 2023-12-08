#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <romcalls.h>

// Implement xv6 exec() in terms of what the kernel provides
int exec(char *path, char *argv[]) {
  int argc;

  // Set argv[0] the same as the path
  argv[0]= path;

  // Count the number of arguments
  for (argc=0; argv[argc] != NULL; argc++) {
cprintf("In lib exec(), argv[%d] is 0x%x\n", argc, argv[argc]);
  }


cprintf("In lib exec(), argc is %d\n", argc);

  return(sys_exec(argc, argv));
}
