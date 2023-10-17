#include <string.h>
#include <stdlib.h>
#include <romcalls.h>

void cprintf(char *fmt, ...);

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    cprintf("Usage: rm files...\n");
    exit(0);
  }

  for(i = 1; i < argc; i++){
    if(sys_unlink(argv[i]) < 0){
      cprintf("rm: %s failed to delete\n", argv[i]);
      break;
    }
  }

  exit(0); return(0);
}
