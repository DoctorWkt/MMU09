#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <xv6/types.h>
#include <xv6/stat.h>
#include <romcalls.h>
extern int errno;

// Perform an xv6 fstat and fill in the fields that xv6 doesn't have
int fstat(int fd, struct stat *statbuf) {
  struct xvstat xvbuf;
  if (statbuf==NULL) {
    errno= EFAULT; return(-1);
  }
  if (sys_fstat(fd, &xvbuf)==-1) {
    errno= EFAULT; return(-1);
  }

  statbuf->st_dev= statbuf->st_rdev= xvbuf.dev;
  statbuf->st_ino= xvbuf.ino;
  statbuf->st_nlink= xvbuf.nlink;
  statbuf->st_size= xvbuf.size;
  statbuf->st_mode= 0777;
  switch (xvbuf.type) {
    case T_DIR: statbuf->st_mode |= S_IFDIR; break;
    case T_DEV: statbuf->st_mode |= S_IFCHR; break;
    default:    statbuf->st_mode |= S_IFREG;
  }
  statbuf->st_uid = statbuf->st_gid = 0;
  statbuf->st_atime = statbuf->st_mtime = statbuf->st_ctime = 0;
  return(0);
}

/* xv6 has no stat(), so we simulate it with fstat() */
int stat(const char *path, struct stat *buf)
{
  int fd, err;
  errno=0;
  fd= open(path, O_RDONLY);
  if (fd==-1) { errno= ENOENT; return(-1); }
  err= fstat(fd, buf);
  close(fd);
  return(err);
}

int lstat(const char *path, struct stat *buf)
{
  return(stat(path,buf));
}
