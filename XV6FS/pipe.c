#include <sys/types.h>
#include <xv6/types.h>
#include <xv6/defs.h>
#include <xv6/param.h>
#include <xv6/proc.h>
#include <xv6/fs.h>
#include <xv6/file.h>

#define PIPESIZE  32
#define PIPECOUNT 16

struct pipe {
  char data[PIPESIZE];
  uint nread;			// number of bytes read
  uint nwrite;			// number of bytes written
  int readopen;			// read fd is still open
  int writeopen;		// write fd is still open
  int inuse;			// If 1, has been allocated
};

// Array of pipes
struct pipe pipelist[PIPECOUNT];

void pipeinit(void) {
  for (int i=0; i< PIPECOUNT; i++)
    pipelist[i].inuse=0;
}

int pipealloc(struct file **f0, struct file **f1) {
  struct pipe *p;

  // Find an unallocated pipe from the list
  p= NULL;
  for (int i=0; i< PIPECOUNT; i++)
    if (pipelist[i].inuse==0) {
      pipelist[i].inuse=1;
      p= &pipelist[i];
      break;
    }

  // No available pipe, give up
  if (p==NULL) goto bad;
  
  // Allocate the file descriptors
  *f0 = *f1 = 0;
  if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  p->readopen = 1;
  p->writeopen = 1;
  p->nwrite = 0;
  p->nread = 0;
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = p;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = p;
  return(0);

bad:
  if (p)
    p->inuse=0;
  if (*f0)
    fileclose(*f0);
  if (*f1)
    fileclose(*f1);
  return(-1);
}

void pipeclose(struct pipe *p, int writable) {
  if (writable) {
    p->writeopen = 0;
    wakeup(&p->nread);
  } else {
    p->readopen = 0;
    wakeup(&p->nwrite);
  }
  if (p->readopen == 0 && p->writeopen == 0)
    p->inuse=0;
}

int pipewrite(struct pipe *p, char *addr, int n) {
  int i;
  char ch;

  for (i = 0; i < n; i++) {
    while (p->nwrite == p->nread + PIPESIZE) {
      if (p->readopen == 0 || curproc->killed) {
	return -1;
      }
      wakeup(&p->nread);
      sleepchan(&p->nwrite);
    }

    // Copy the data from userspace
    rommemcpy(1, &addr[i], &ch);
    p->data[p->nwrite++ % PIPESIZE] = ch;
  }
  wakeup(&p->nread);
  return n;
}

int piperead(struct pipe *p, char *addr, int n) {
  int i;
  char ch;

  while (p->nread == p->nwrite && p->writeopen) {
    if (curproc->killed) {
      return -1;
    }
    sleepchan(&p->nread);
  }
  for (i = 0; i < n; i++) {
    if (p->nread == p->nwrite)
      break;
    // Copy the data to userspace
    ch = p->data[p->nread++ % PIPESIZE];
    rommemcpy(1, &ch, &addr[i]);
  }
  wakeup(&p->nwrite);
  return i;
}
