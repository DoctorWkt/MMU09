#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

// (c) 2023, Warren Toomey, BSD license

int main(int argc, char *argv[])
{
  struct stat sb;
  char *src;
  char *dest;
  char destdir[PATH_MAX];

  if (argc != 3) { fprintf(stderr, "Usage: mv old new\n"); exit(0); }
  src= argv[1]; dest= argv[2];

  // Is the destination a directory?
  if (stat(dest, &sb)==0) {
    if (S_ISDIR(sb.st_mode)) {
      // Yes, join dest directory and source together
      snprintf(destdir, PATH_MAX, "%s/%s", dest, src);
      dest= destdir;
    } else { fprintf(stderr, "%s already exists\n", dest); exit(0); }
  }

  // Now try to do the link
  if (link(src, dest) < 0) {
    fprintf(stderr, "link %s %s failed\n", src, dest); exit(1); 
  }

  // Now try to do the source unlink
  if (unlink(src) < 0) {
    fprintf(stderr, "unlink %s failed\n", src); exit(1); 
  }
  exit(0); return(0);
}
