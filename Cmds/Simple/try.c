#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <romcalls.h>
#include <termios.h>

void cprintf(char *fmt, ...);

char nl= 0x0A;

int main() {
  char *start= (char *)0x3db0;
  char *end= (char *)0x3def;
  char *cstart= (char *)0x4db0;
  char *cend= (char *)0x4def;
  char *cptr;
  char ch= ' ';

  
  // Store stuff
  for (cptr= start; cptr < end; cptr++)
    *cptr= ch++;

  // Print stuff to ensure it's OK
  for (cptr= start; cptr < end; cptr++) {
    romputc(*cptr & 0xff);
  }

  // Wait for a keypress
  romgetputc();

  // Loop printing it out, which somehow alters the data
  while (1) {
    for (cptr= start; cptr < end; cptr++) {
      cprintf("%x %x\n", cptr, *cptr & 0xff);
    }
    for (cptr= start; cptr < end; cptr++) {
      romputc(*cptr & 0xff);
    }
    romputc('\n');

    // Copy it
    memcpy(cstart, start, end-start+1);
    // Print the copy out
    for (cptr= cstart; cptr < cend; cptr++) {
      romputc(*cptr & 0xff);
    }
    romputc('\n');
  }
  _exit(0);
}
