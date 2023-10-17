#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <xv6/param.h>
#include <fcntl.h>
#include <romcalls.h>
#include <readline/readline.h>
void cprintf(char *fmt, ...);

char * addr= 0x0000;
int i, j;

int main() {
  for (i=0; i < 0xfe00; i++, addr++) {
    j= *addr & 0xff;
    cprintf("%x: %x\n", i, j);
  }

  while (1) ;
}
