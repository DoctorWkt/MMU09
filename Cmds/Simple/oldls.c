#include <stdlib.h>
#include <string.h>
#include <xv6/types.h>
#include <xv6/stat.h>
#include <xv6/fcntl.h>
#include <xv6/fs.h>
#include <romcalls.h>

void cprintf(char *fmt, ...);

void panic(char *str) {
  while (*str) { romputc(*str); str++; }
  romputc('\n'); exit(1);
}

int
sys_stat(char *n, struct xvstat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = sys_fstat(fd, st);
  close(fd);
  return r;
}

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct xvdirent de;
  struct xvstat st;

  if((fd = open(path, 0)) < 0){
    cprintf("ls: cannot open %s\n", path);
    return;
  }

  if(sys_fstat(fd, &st) < 0){
    cprintf("ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    cprintf("%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      cprintf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, (char *) &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(sys_stat(buf, &st) < 0){
        cprintf("ls: cannot stat %s\n", buf);
        continue;
      }
      cprintf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit(0);
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit(0); return(0);
}
