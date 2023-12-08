// Code for process manipulation, process scheduling and
// page/frame management. Some code from xv6.
// (c) 2023 Warren Toomey, BSD license.

#include <sys/types.h>
#include <string.h>
#include <xv6/types.h>
#include <xv6/param.h>
#include <xv6/defs.h>
#include <xv6/fs.h>
#include <xv6/fcntl.h>
#include <xv6/proc.h>

#define STACKTOP 0xFDFD		// The top of the user's stack
#define USERCODE 0x0002		// The start of the user's code
#define USERDATA 0x2000		// User data starts here or higher up

extern volatile char uframe0;

// The process table
struct proc ptable[NPROC];

// The currently-running process
struct proc *curproc;

// The initial process, i.e. init
static struct proc *initproc;

// The next process-id
int nextpid;

// The list of free/in-use page frames
char inuseframe[NFRAMES];

// The I/O locations to set the page table entries
volatile char *pte0;
volatile char *pte1;
volatile char *pte2;
volatile char *pte3;
volatile char *pte4;
volatile char *pte5;
volatile char *pte6;
volatile char *pte7;

// Allocate an unused page frame.
// Panic otherwise.
static char allocframe(void)
{
  int i;

  for (i=0; i<NFRAMES; i++)
    if (inuseframe[i]==0) {
      inuseframe[i]=1;
      return(i);
    }
  panic("no free page frames");
}

// Deallocate an in-use page frame.
// Panic if not currently in-use.
static void freeframe(char fnum)
{
  if (fnum < 0 || fnum >= NFRAMES)
    panic("bad fnum in freeframe()");

  if (inuseframe[fnum]==0)
    panic("freeing already free page frame");

  inuseframe[fnum]=0;
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and return
// a pointer to the entry. Otherwise return NULL.
static struct proc* allocproc(void)
{
  struct proc *p;

  for (p = ptable; p < &ptable[NPROC]; p++)
    if (p->state == UNUSED) {
      p->state = EMBRYO;
      p->pid = nextpid++;
      return(p);
    }

  return(NULL);
}

// Given the base address of a page already
// in memory, and a newly-allocate frame number,
// map the frame into memory and copy the
// page into the new frame.
void copypage(char *from, char toframe)
{
  int length= PGSIZE;
  volatile char *mappage= pte1;		// Where we will map the new frame
  int origframe= curproc->frame[1];	// The frame that is currently there
  int page2frame= -1;

  // We try to use the page at $2000 to map in the new
  // frame as we can use rommemcpy() and hide the ROM.
  char *to= (char *)0x2000;

  // If we are copying from $2000, we can't put the
  // new frame at $2000. Use $4000 instead.
  if (from == (char *)0x2000) {
    mappage= pte2;
    origframe= curproc->frame[2];
    to= (char *)0x4000;

  // For the top-most page, we have to be
  // careful and not copy $FE00 to $FFFF.
  } else if (from == (char *)0xe000) {
    length -= 512;

  // The lowest page currently has the kernel data.
  // So, map the user's page in at $4000 and copy from there.
  // Record which frame got mapped out in the process.
  } else if (from == (char *)0x0000) {
    page2frame= curproc->frame[2];
    *pte2= curproc->frame[0];
    from= (char *)0x4000;
  }

  // Map the new frame into memory
  *mappage= toframe;

  // Copy from the original page to the new frame
  rommemcpy(length, from, to);

  // Map page 2 back in if needed
  if (page2frame != -1)
    *pte2= page2frame;

  // Map the previous frame into memory
  *mappage= origframe;
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
  struct proc *p;

  for(p = ptable; p < &ptable[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Gloabl used by sched() and fork()
int _schedsp;

// Create a new runnable process by copying
// the current process. Return the pid of
// the new process, or -1 if the fork fails.
int fork1(void)
{
  struct proc *np;
  int i;
  int from;

  // Allocate a process.
  if((np = allocproc()) == NULL)
    return -1;

  // Set the new process' parent
  np->parent= curproc;

  // Save the stack pointer in the new process.
  np->usersp= _schedsp;
// cprintf("Saved sp 0x%x for pid %d\n", np->usersp, np->pid);

  // Allocate the process' memory
  for (i=0; i< NPAGES; i++)
    np->frame[i]= allocframe();

  // Copy the memory from parent to child
  for (i=0, from=0; i< NPAGES; i++, from+= PGSIZE) {
    copypage((char *)from, np->frame[i]);
  }

  // Duplicate the file descriptors and cwd
  for (i = 0; i < NOFILE; i++)
    if (curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  // Mark as not sleeping or killed
  np->chan= NULL;
  np->killed= 0;

  // Copy the process name
  rommemcpy(sizeof(curproc->name), curproc->name, np->name);

  // Mark as runnable and return the pid
  np->state = RUNNABLE;
  return(np->pid);
}

// A buffer to hold a copy of the string arguments
// in some syscall arguments.
char userbuf[512];

// Buffer to build argv[] and strings before we
// copy them up to the top of stack
char execbuf[512];

// Replace the currently-running program with the
// program named in argv[0]. Copy the argv[]
// structure to the top of the program's stack
// so that it is visible to the new program.
// Return if the exec() fails.
void exec(int argc, char *argv[]) {
  int i, stroffset, memsize, len, fd;
  char **oldargv, **newargv, **newarglist;
  char *destbuf, *sptr, *destsptr, *progname;

  // Error if argv points nowhere
  if (argv==NULL) return;
// cprintf("In exec, argv lives at %x and points at %x\n", &argv, argv);

  // Calculate the amount of memory to allocate:
  // argc+1 times char pointers. The +1
  // allows us to put a NULL on the end of newargv.
  // Also allow for the pointer to the arglist before newargv.
  // Save the offset to the future first string location.
  stroffset = memsize = sizeof(char **) + (argc + 1) * sizeof(char *);
// cprintf("Size of argv[] pointers: %d\n", memsize);

  // Copy the argv pointers into the userbuf
  if (memsize >= 512) return;
  romstrncpy(512, argv, userbuf);
  oldargv= (char **)userbuf;

#if 0
  // Debug: print out arguments. Copy them first.
  cprintf("argc %d, args are:\n", argc);
  sptr= userbuf; sptr += memsize;
  for (i=0; i<argc; i++) {
    romstrncpy(512, oldargv[i], sptr);
    cprintf("  %s\n", sptr);
  }
#endif

  // Now add on the lengths of all the arguments.
  // Ensure we count the NUL at the end of each argument.
  for (i = 0; i < argc; i++) {
    memsize += romstrlen(oldargv[i]) + 1;
  }
// cprintf("Size of argv[] pointers and strings: %d\n", memsize);

  // Set the location of the new argv and arglist in the execbuf
  newargv = (char **) &execbuf[0];
  newarglist = (char **) &execbuf[2];

  // Set the base of the strings. Keep a copy of
  // it as the program name to open later.
  progname= sptr = (char *)(&execbuf[0] + stroffset);

  // Determine where the execbuf will end up on the stack
  // and where the strings will start there. Point newargv
  // at where the arg list will end up on the stack.
  destbuf = (char *) (STACKTOP - memsize);
  destsptr= (char *)(destbuf + stroffset);
  *newargv= (char *)destbuf + 2;

#if 0
  // Debug
  cprintf("newargv is %x and points at %x\n", newargv, *newargv);
  cprintf("newarglist is %x\n", newarglist);
  cprintf("sptr is %x\n", sptr);
  cprintf("destbuf is %x\n", destbuf);
  cprintf("destsptr is %x\n", destsptr);
#endif

  // Copy the arguments into the execbuf.
  // Work out where the arguments will live
  // on the stack and store their pointers
  // in newarglist.
  for (i = 0; i < argc; i++) {
    len = romstrlen(oldargv[i]) + 1;
    rommemcpy(len, oldargv[i], sptr);
    newarglist[i]= destsptr;
// cprintf("  Copied %s\n", sptr);
// cprintf("  Pointer on stack for it: %x\n", destsptr);
    sptr += len;
    destsptr += len;
  }

  // Put a NULL at the end of the newarglist array
  newarglist[i]= NULL;

  // Open the program read-only. Return if we can't open it.
// cprintf("About to open %s in exec()\n", progname);
  if ((fd = kopen(progname, O_RDONLY)) < 0) return;

  // Put the user's frame[0] into page 1, as it will eventually
  // become page 0, and it will allow us (the kernel) to
  // write to it.
  *pte1= curproc->frame[0];
// cprintf("Copying first page into frame %d\n", curproc->frame[0]);

  // Copy the file into memory but at page 1 not page 0!
  sptr= (char *)USERCODE + PGSIZE;

  // Read the first 8190 (USERDATA - USERCODE) bytes into this page.
  // Then map in the process' frame 1 into page 1
  i= kread(fd, sptr, USERDATA - USERCODE);
  *pte1= curproc->frame[1];

  // Now read the rest of the file into memory. I'm pointing
  // sptr at USERDATA just to ensure we won't write on the
  // kernel data accidentally.
  sptr= (char *)USERDATA;
  while ((i = kread(fd, sptr, BSIZE)) > 0) sptr += BSIZE;
  sys_close(fd);

  // We now have the argv[] in execbuf ready to copy up to the top of the stack.
  // Use an assembly routine to do this, and then start the code running.
  // Set up the knowledge of the frame at page zero beforehand.
  uframe0 = curproc->frame[0];
// cprintf("Uframe0 %x, doing jmptouser(%d, %d, %x)\n", uframe0, memsize, argc, destbuf);
  jmptouser(memsize, argc, destbuf);
}

// Schedule a process. Return with the
// process' context mapped in to memory.
// We also switch from the old to the
// new stack. This is fine as the new
// process had also called sched.
// The real interface to this is sched()
// which is in romfuncs.s.
void sched1(void)
{
  struct proc *p;

// cprintf("Entered sched()\n");

  // Loop indefinitely. This makes us the null process,
  // busy-waiting until some blocked process wakes up.
  while (1) {

    // Start one past the current process, so that
    // we don't keep scheduling the current process.
    p= curproc; p++;
    while (1) {

      // Wrap around to the start of the proc table if required
      if (p== &ptable[NPROC]) p= ptable;

      // Leave if we got back to the current process
      // and it is not runnable
      if (p==curproc && p->state != RUNNABLE) break;

      // Make this the current process if it is runnable
      if (p->state == RUNNABLE) {
	// Save the stack pointer in the process slot
	curproc->usersp= _schedsp;
// cprintf("Saved sp 0x%x for pid %d\n", curproc->usersp, curproc->pid);

	// Make the new process the current process
	curproc= p;

	// Map in the new process' memory.
	// As the kernel is using page zero,
	// we save ours to uframe0 and let the
	// assembly code map it to page zero before we RTI.
// cprintf("Setting pid %d stack pointer to 0x%x\n", curproc->pid, _schedsp);
	uframe0 = curproc->frame[0];
	*pte1  = curproc->frame[1];
	*pte2  = curproc->frame[2];
	*pte3  = curproc->frame[3];
	*pte4  = curproc->frame[4];
	*pte5  = curproc->frame[5];
	*pte6  = curproc->frame[6];
	*pte7  = curproc->frame[7];

	// Set up the new process' stack pointer.
	_schedsp= curproc->usersp;
        __asm("\tlds\t_schedsp");

	// This returns pid zero to a forked child
        __asm("\tldd\t#0");
        __asm("\trts");
	return;
      }

      p++;	// Move to the next process slot
    }
  }
}

// Exit a process. If thisproc is NULL, exit the current
// process, do not return and schedule another process.
// If thisproc is non-NULL, exit thisproc and return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
static void
exitproc(int exitvalue, struct proc *thisproc)
{
  int i, fd;
  struct proc *p;

  // Exit the current process if thisproc is NULL
  if (thisproc==NULL) thisproc= curproc;

  // Panic if init exits!
  if (thisproc == initproc)
    panic("init exiting");

  // Save the exit value
  thisproc->exitstatus= exitvalue & 0xff;

  // Free the page frames
  for (i=0; i< NPAGES; i++)
    freeframe(thisproc->frame[i]);

  // Close all open files.
  for (fd = 0; fd < NOFILE; fd++){
    if (thisproc->ofile[fd]){
      fileclose(thisproc->ofile[fd]);
      thisproc->ofile[fd] = 0;
    }
  }

  // Detach from the cwd
  iput(thisproc->cwd);
  thisproc->cwd = 0;

  // Mark the process as a zombie
  thisproc->state = ZOMBIE;

  // Wake the parent if it is sleeping
  wakeup(thisproc->parent);

  // Pass any abandoned children to init
  for (p = ptable; p < &ptable[NPROC]; p++) {
    if (p->parent == thisproc) {
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup(initproc);
    }
  }

  // If the current process exited,
  // jump into the scheduler, never to return.
  if (thisproc == curproc) {
    sched();
    panic("zombie exit");
  }
}

// Exit the current process by calling exitproc().
void
sys_exit(int exitvalue)
{ exitproc(exitvalue, NULL);
}

// Sleep on chan.
void
sleepchan(void *chan)
{
  if(curproc == 0)
    panic("sleep");

  // Go to sleep.
  curproc->chan = chan;
  curproc->state = SLEEPING;
  sched();

  // Tidy up.
  curproc->chan = 0;
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
// Return the child's exit status in the optional pointer argument
int
sys_wait(int *statusptr)
{
  struct proc *p;
  int havekids, pid;

  while (1) {
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable; p < &ptable[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
	if(statusptr) {
	  *statusptr= p->exitstatus | (p->killed ? 0x100 : 0);
	}
        p->killed = 0;
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    // XXX Why killed on the next line?
    if(!havekids || curproc->killed){
      return -1;
    }

    // Wait for children to exit.
    sleepchan(curproc);
  }
}

// Initialise the process and memory data structures,
// then start the initial process.
void procinit(void)
{
  int i,fd;

  // These strings are on the stack and not in ROM, because
  // they will get copied with romstrncpy() and, in the
  // process, the ROM is mapped out. So they can't be in ROM!

  // The name of the console device
  char ttystr[]= { '/', 't', 't', 'y', 0 };

  // The program to run
  char shell[]= { '/', 'b', 'i', 'n', '/', 'i', 'n', 'i', 't', 0 };

  // The initial argv
  char *argv[]= { shell, NULL };

  // Set up the page table locations
  pte0= (char *)0xfe70;
  pte1= (char *)0xfe71;
  pte2= (char *)0xfe72;
  pte3= (char *)0xfe73;
  pte4= (char *)0xfe74;
  pte5= (char *)0xfe75;
  pte6= (char *)0xfe76;
  pte7= (char *)0xfe77;

  // Clear the process table
  memset(ptable, 0, NPROC * sizeof(struct proc));

  // Mark all page frames available except frame 0,
  // which is used for kernel data.
  memset(inuseframe, 0, NFRAMES);
  inuseframe[0]= 1;

  // Initialise nextpid
  nextpid=1;

  // Allocate the initial process slot
  initproc= curproc= allocproc();

  // Allocate the process' memory
  for (i=0; i< NPAGES; i++) {
    curproc->frame[i]= allocframe();
  }

  // Map in the new process' memory.
  // The exec() will deal with page zero.
  *pte1  = curproc->frame[1];
  *pte2  = curproc->frame[2];
  *pte3  = curproc->frame[3];
  *pte4  = curproc->frame[4];
  *pte5  = curproc->frame[5];
  *pte6  = curproc->frame[6];
  *pte7  = curproc->frame[7];

  // Now open fds 0, 1 and 2 to the console
  fd = kopen(ttystr, O_RDONLY);
  fd = kopen(ttystr, O_WRONLY);
  fd = kopen(ttystr, O_WRONLY);

  // Set up the working directory
  curproc->cwd = namei("/");

  // Start the intial process
  exec(1, argv);
}

// The userland interface to exec()
void sys_exec(int argc, long d0, long d1, long d2, int d3, char *argv[]) {
  exec(argc, argv);
}

// Get the current process-id
int sys_getpid(void)
{
  return(curproc->pid);
}


// Kill the process with the given pid.
int kkill(int pid)
{
  struct proc *p;

  for (p = ptable; p < &ptable[NPROC]; p++) {
    if(p->pid == pid){
      // Mark it as killed, then call exitproc()
      // to free all the resources
      p->killed = 1;
      exitproc(1, p);
      return(0);
    }
  }
  return(-1);
}

// kill() system call. Note that there is no second
// argument. We do an implicit SIGKILL.
int sys_kill(int pid)
{
  if (pid < 0)
    return(-1);
  return kkill(pid);
}
