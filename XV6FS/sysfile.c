//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//
// Note that system calls with multiple arguments
// have dummy arguments d1 .. d4. These "pad out"
// the SWI stack frame so the functions see the
// real arguments.

#include <string.h>
#include <errno.h>
#include <xv6/types.h>
#include <xv6/defs.h>
#include <xv6/param.h>
#include <xv6/stat.h>
#include <xv6/fs.h>
#include <xv6/file.h>
#include <xv6/fcntl.h>
#include <xv6/proc.h>

#define SBUFSIZE 100
char seekbuf[SBUFSIZE];		// Buffer to do lseek()

int errno;			// The kernel location of errno

void set_errno(Int err) {	// and the code to set it
  errno= err;
}

// Given a file descriptor
// return both the descriptor and the corresponding struct file.
static int argfd(Int fd, Int * pfd, struct file **pf) {
  struct file *f;

  if (fd < 0 || fd >= NOFILE || (f = curproc->ofile[fd]) == 0) {
    set_errno(EBADF); return(-1);
  }
  if (pfd) *pfd = fd;
  if (pf) *pf = f;
  return(0);
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int fdalloc(struct file *f) {
  Int fd;

  for (fd = 0; fd < NOFILE; fd++) {
    if (curproc->ofile[fd] == 0) {
      curproc->ofile[fd] = f;
      return fd;
    }
  }
  set_errno(EMFILE);
  return -1;
}

Int sys_dup(Int fd) {
  struct file *f;

  set_errno(0);
  if (argfd(fd, 0, &f) < 0)
    return -1;
  if ((fd = fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

Int kread(Int fd, char *p, Int n) {
  struct file *f;
  char kp;

  set_errno(0);
  if (argfd(fd, 0, &f) < 0 || n < 0 || p == 0)
    return -1;

  // If the file is the console, return one character from the UART.
  // Copy it from kernel space to userspace.
  if (f->type == FD_CONSOLE) {
    
    kp= (char)(romgetputc() & 0xff);
    rommemcpy(1, &kp, p); return(1);
  }

  // Otherwise read from a file
  return fileread(f, p, n);
}

Int sys_read(Int fd, long d1, long d2, long d3, int d4, char *p, Int n) {
  return(kread(fd, p, n));
}

Int kwrite(Int fd, char *p, Int n) {
  struct file *f;
  int i;
  char kch;

  set_errno(0);
  if (argfd(fd, 0, &f) < 0 || n < 0 || p == 0)
    return -1;

  // If the file is the console, send the characters here.
  // Copy from userspace to kernel space first.
  if (f->type == FD_CONSOLE) {
    for (i=0; i<n; i++) {
      rommemcpy(1, &p[i], &kch);
      romputc(kch);
    }
    return(n);
  } else
    return (Int) filewrite(f, p, n);
}

Int sys_write(Int fd, long d1, long d2, long d3, int d4, char *p, Int n) {
  return(kwrite(fd, p, n));
}

Int sys_close(Int fd) {
  struct file *f;

  set_errno(0);
  if (argfd(fd, 0, &f) < 0)
    return -1;
  curproc->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

// The st argument lives in userspace and we might
// not be able to see it. We fill in our own struct
// and then copy it to the st argument.
Int sys_fstat(Int fd, long d1, long d2, long d3, int d4, struct xvstat *st) {
  struct file *f;
  struct xvstat kernst;
  Int result;

  set_errno(0);
  if (argfd(fd, 0, &f) < 0 || st == 0)
    return -1;
  // If the file is the console, set this type in the xvstat struct
  if (f->type == FD_CONSOLE) {
    kernst.type= T_DEV; result=0;
  } else {
    result= filestat(f, &kernst);
  }
  rommemcpy(sizeof(struct xvstat), &kernst, st);
  return(result);
}

// Create the path new as a link to the same inode as old.
Int sys_link(char *old, long d1, long d2, long d3, int d4, char *new) {
  char name[DIRSIZ];
  struct inode *dp, *ip;

  set_errno(0);
  if (old == 0 || new == 0) {
    set_errno(ENOENT);
    return -1;
  }

  // Copy the old filename to a kernel buffer
  romstrncpy(512, old, userbuf);

  // begin_op();
  if ((ip = namei(userbuf)) == 0) {
    set_errno(ENOENT);
    return -1;
  }

  ilock(ip);
  if (ip->type == T_DIR) {
    iunlockput(ip);
    set_errno(EPERM);
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  // Copy the new filename to a kernel buffer
  romstrncpy(512, new, userbuf);

  if ((dp = nameiparent(userbuf, name)) == 0)
    goto bad;
  ilock(dp);
  if (dirlink(dp, userbuf, ip->inum) < 0) {
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  set_errno(EEXIST);
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int isdirempty(struct inode *dp) {
  xvoff_t off;
  struct xvdirent de;

  for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de)) {
    if (readi(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
      panic("sy1");
    if (de.inum != 0)
      return 0;
  }
  return 1;
}

Int sys_unlink(char *path) {
  struct inode *ip, *dp;
  struct xvdirent de;
  char name[DIRSIZ];
  xvoff_t off;

  set_errno(0);
  if (path == 0) {
    set_errno(ENOENT);
    return -1;
  }

  // Copy the path to a kernel buffer
  romstrncpy(512, path, userbuf);

  if ((dp = nameiparent(userbuf, name)) == 0) {
    set_errno(EPERM);
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if (namecmp(userbuf, ".") == 0 || namecmp(userbuf, "..") == 0)
    goto bad;

  if ((ip = dirlookup(dp, userbuf, &off)) == 0)
    goto bad;
  ilock(ip);

  if (ip->nlink < 1)
    panic("sy2");
  if (ip->type == T_DIR && !isdirempty(ip)) {
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if (writei(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
    panic("sy3");
  if (ip->type == T_DIR) {
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  return 0;

bad:
  iunlockput(dp);
  set_errno(EPERM);
  return -1;
}

static struct inode *create(char *path, short type) {
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if ((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if ((ip = dirlookup(dp, name, 0)) != 0) {
    iunlockput(dp);
    ilock(ip);
    if (type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if ((ip = ialloc(type)) == 0)
    panic("sy4");

  ilock(ip);
  ip->nlink = 1;
  iupdate(ip);

  if (type == T_DIR) {		// Create . and .. entries.
    dp->nlink++;		// for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("sy5");
  }

  if (dirlink(dp, name, ip->inum) < 0)
    panic("sy6");

  iunlockput(dp);

  return ip;
}

void itrunc(struct inode *);

Int kopen(char *path, Int omode) {
  Int fd;
  Int type= FD_INODE;
  struct file *f;
  struct inode *ip;

  set_errno(0);
  if (path == 0 || omode < 0) {
    set_errno(ENOENT);
    return -1;
  }

  // Copy the path to a kernel buffer
  romstrncpy(512, path, userbuf);

  // If the filename is "/tty", make a console file descriptor
  if (!strncmp(userbuf, "/tty", 4)) {
    type= FD_CONSOLE;
  } else {

    if (omode & O_CREATE) {
      ip = create(userbuf, T_FILE);
      if (ip == 0) {
  	set_errno(EEXIST);
        return -1;
      }
    } else {
      if ((ip = namei(userbuf)) == 0) {
  	set_errno(ENOENT);
        return -1;
      }
      ilock(ip);
      if (ip->type == T_DIR && omode != O_RDONLY) {
        iunlockput(ip);
  	set_errno(EISDIR);
        return -1;
      }
    }
  }

  if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0) {
    if (f)
      fileclose(f);
    iunlockput(ip);
    set_errno(EACCES);
    return -1;
  }

  if((omode & O_TRUNC)) {
    itrunc(ip);
  }
  iunlock(ip);

  f->type = type;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  if(omode & O_APPEND)
    f->off= f->ip->size;
  return fd;
}

Int sys_open(char *path, long d1, long d2, long d3, int d4, Int omode) {
  return(kopen(path, omode));
}

Int sys_mkdir(char *path) {
  struct inode *ip;

  set_errno(0);
  if (path == 0) {
    set_errno(EINVAL);
    return -1;
  }

  // Copy the path to a kernel buffer
  romstrncpy(512, path, userbuf);

  if ((ip = create(userbuf, T_DIR)) == 0) {
    set_errno(EINVAL);
    return -1;
  }
  iunlockput(ip);
  return 0;
}

Int sys_chdir(char *path) {
  struct inode *ip;

  set_errno(0);
  if (path == 0) {
    set_errno(EINVAL);
    return -1;
  }

  // Copy the path to a kernel buffer
  romstrncpy(512, path, userbuf);

  if ((ip = namei(userbuf)) == 0) {
    set_errno(EINVAL);
    return -1;
  }
  ilock(ip);
  if (ip->type != T_DIR) {
    iunlockput(ip);
    set_errno(ENOTDIR);
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  curproc->cwd = ip;
  return 0;
}

// lseek code derived from https://github.com/ctdk/xv6
xvoff_t sys_lseek(Int fd, long d1, long d2, long d3, int d4, xvoff_t offset, Int base)
{
        xvoff_t newoff;
        xvoff_t zerosize;

        struct file *f;

	set_errno(0);
        if ((argfd(fd, 0, &f)<0) || offset<0 || base<0) {
          set_errno(EINVAL);
          return(-1);
  	}

        if( base == SEEK_SET)
                newoff = offset;

        if (base == SEEK_CUR)
                newoff = f->off + offset;

        if (base == SEEK_END)
                newoff = f->ip->size + offset;

        if (newoff < 0) {
	  set_errno(EINVAL);
          return -1;
	}

	// If the new offset is past the file's current size
        if (newoff > f->ip->size){
		// Work out how much to add to the file
		// and fill a buffer with zeroes
                zerosize = newoff - f->ip->size;
		memset(seekbuf, 0, SBUFSIZE);

		// Write the buffer until we have reached the new size
                while (zerosize > 0){
                        filewrite(f, seekbuf, zerosize);
                        zerosize -= SBUFSIZE;
                }
        }

        f->off = newoff;
        return newoff;
}

int sys_pipe(int *fd)
{
  struct file *rf, *wf;
  int fd0, fd1;

  // Error if fd points nowhere
  if (fd==NULL) return(-1);

  // Allocate a pipe struct
  if(pipealloc(&rf, &wf) < 0)
    return(-1);

  // Allocate file descriptors
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      curproc->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return(-1);
  }

  // Copy the fds to userland
  rommemcpy(2, &fd0, &fd[0]);
  rommemcpy(2, &fd1, &fd[1]);
  return(0);
}

// Intialise the filesystem data structures
void sys_init(void) {
  ch375init();			// USB key
  binit();			// Buffer cache
  fileinit();			// File table
  iinit();			// Inode table
  pipeinit();			// Pipe list
  procinit();			// Launch the first process
}
