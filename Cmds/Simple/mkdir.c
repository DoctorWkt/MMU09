#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xv6/types.h>
#include <xv6/stat.h>
#include <xv6/fcntl.h>
#include <xv6/fs.h>
#include <romcalls.h>

void cprintf(char *fmt, ...);

int main(int argc, char *argv[]) {
  int i;

  if (argc < 2) {
    cprintf("Usage: mkdir files...\n");
    exit(1);
  }

  for (i = 1; i < argc; i++) {
    if (sys_mkdir(argv[i]) < 0) {
      cprintf("mkdir: %s failed to create\n", argv[i]);
      break;
    }
  }

  exit(0); return(0);
}
