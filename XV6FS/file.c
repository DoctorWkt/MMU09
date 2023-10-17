//
// File descriptors
//

#include <sys/types.h>
#include <errno.h>
#include <xv6/types.h>
#include <xv6/defs.h>
#include <xv6/param.h>
#include <xv6/fs.h>
#include <xv6/file.h>

struct {
  struct file file[NFILE];
} ftable;

void fileinit(void) {
}

// Allocate a file structure.
struct file *filealloc(void) {
  struct file *f;

  for (f = ftable.file; f < ftable.file + NFILE; f++) {
    if (f->ref == 0) {
      f->ref = 1;
      return f;
    }
  }
  return 0;
}

// Increment ref count for file f.
struct file *filedup(struct file *f) {
  if (f->ref < 1)
    panic("fi1");
  f->ref++;
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void fileclose(struct file *f) {
  struct file ff;

  if (f->ref < 1)
    panic("fi2");
  if (--f->ref > 0) {
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;

  if (ff.type == FD_INODE) {
    iput(ff.ip);
  }
}

// Get metadata about file f.
Int filestat(struct file *f, struct xvstat *st) {
  if (f->type == FD_INODE) {
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  set_errno(EINVAL);
  return -1;
}

// Read from file f.
Int fileread(struct file *f, char *addr, xvoff_t n) {
  xvoff_t r;

  if (f->readable == 0) {
    set_errno(EINVAL);
    return -1;
  }
  if (f->type == FD_INODE) {
    ilock(f->ip);
    if ((r = readi(f->ip, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
    return r;
  }
  panic("fi3");
  return (0);			// Keep -Wall happy
}

//PAGEBREAK!
// Write to file f.
xvoff_t filewrite(struct file *f, char *addr, xvoff_t n) {
  xvoff_t r;

  if (f->writable == 0) {
    set_errno(EINVAL);
    return -1;
  }
  if (f->type == FD_INODE) {
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    xvblk_t max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * 512;
    xvoff_t i = 0;
    while (i < n) {
      xvoff_t n1 = n - i;
      if (n1 > max)
	n1 = max;

      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
	f->off += r;
      iunlock(f->ip);

      if (r < 0)
	break;
      if (r != n1)
	panic("fi4");
      i += r;
    }
    return i == n ? n : (xvoff_t) - 1;
  }
  panic("fi5");
  return (0);			// Keep -Wall happy
}
