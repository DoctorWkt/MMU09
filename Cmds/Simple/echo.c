#include <string.h>
#include <unistd.h>
void cprintf(char *fmt, ...);

int main(int argc, char *argv[]) {
  int i;

  for (i = 1; i<argc; i++) {
    if (argv[i] == NULL) break;
    //cprintf("%s ", argv[i]);
    write(1, argv[i], strlen(argv[i]));
    write(1, " ", 1);
  }
  // romputc('\n');
  write(1, "\n", 1);
  close(1);

  exit(0); return(0);
}
