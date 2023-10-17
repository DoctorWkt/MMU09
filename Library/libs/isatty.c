// isatty.c: If the file is a character device, it has to be the tty

#include <sys/stat.h>

int isatty(int fd)
{
  struct stat statbuf;

  if (fstat(fd, &statbuf)== -1) return(0);
  if (statbuf.st_mode & S_IFCHR) return(1);
  return(0);
}
