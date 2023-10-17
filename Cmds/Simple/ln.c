#include <unistd.h>
void cprintf(char *fmt, ...);

int
main(int argc, char *argv[])
{
  if(argc != 3){
    cprintf("Usage: ln old new\n");
    exit(0);
  }
  if(link(argv[1], argv[2]) < 0)
    cprintf("link %s %s: failed\n", argv[1], argv[2]);
  exit(0); return(0);
}
