// Code to deal with starting and ending user programs

#include <string.h>
#include <xv6/types.h>
#include <xv6/defs.h>
#include <xv6/param.h>
#include <xv6/stat.h>
#include <xv6/fs.h>
#include <xv6/file.h>
#include <xv6/fcntl.h>

#define STACKTOP 0xFDFD		// The top of the user's stack
#define USERCODE 0x0002		// The start of the user's code
#define USERDATA 0x2000		// User data starts here or higher up
#define PAGESIZE 0x2000		// The size of our pages

// A buffer to hold a copy of the string arguments
// in some syscall arguments.
char userbuf[512];

// The argv[] used to start "/bin/sh" from exit()
extern char *initargv[];

// Replace the currently-running program with the
// program named in argv[0]. Copy the argv[]
// structure to the top of the program's stack
// so that it is visible to the new program.
// Open up fd's 0, 1 and 2 to the console.
// Can return if the spawn() fails.
void sys_spawn(int argc, char *argv[]) {
  int i, fd, len, stroffset, memsize;
  char *destbuf, *sptr;
  char **newargv, **newarglist;

  // This string is on the stack and not in ROM, because
  // it will get copied with romstrncpy() and, in the
  // process, the ROM is mapped out. So it can't be in ROM!
  //
  // The name of the console device
  char ttystr[]= { '/', 't', 't', 'y', 0 };


  // Error if argv points nowhere
  if (argv==NULL) return;

// cprintf("argc %d\n", argc);
// for (i=0; i<argc; i++) cprintf("  %s\n", argv[i]);
// cprintf("\n");

  // Calculate the amount of memory to allocate:
  // argc+1 times char pointers. The +1
  // allows us to put a NULL on the end of newargv.
  // Also allow for the pointer to the arglist before newargv.
  // Save the offset to the future first string location.
  stroffset = memsize = sizeof(char **) + (argc + 1) * sizeof(char *);

  // Now add on the lengths of all the arguments.
  // Ensure we count the NUL at the end of each argument.
  for (i = 0; i < argc; i++) {
    memsize += romstrlen(argv[i]) + 1;
  }

  // Set the start of the destbuf at
  // the top of the stack minus memsize.
  destbuf = (char *) (STACKTOP - memsize);

  // Set the location of the new argv and arglist in userspace
  newargv = (char **) destbuf;
  newarglist = (char **) (destbuf + sizeof(char **));

  // Set the base of the strings.
  sptr = destbuf + stroffset;

  // Copy the arguments and put pointers into newargv.
  // Make sure we copy the NUL at the end of each argument.
  // Also copy argv[0] down to kernel memory
  for (i = 0; i < argc; i++) {
    len = romstrlen(argv[i]) + 1;
    rommemcpy(len, argv[i], sptr);
    rommemcpy(sizeof(char *), &sptr, &newarglist[i]);
    if (i==0) rommemcpy(len, argv[i], userbuf);
    sptr += len;
  }

  // Put the pointer to newarglist into newargv
  rommemcpy(sizeof(char **), &newarglist, newargv);

  // Put a NULL at the end of the newarglist array
  sptr=NULL;
  rommemcpy(sizeof(char *), &sptr, &newarglist[i]);
  // panic("foo");

  // We have a copy of the program's name in userbuf.
  // Open it read-only. Return if we can't open it.
  if ((fd = sys_open(userbuf, O_RDONLY)) < 0) return;

  // Put page frame 8 into page 1, as it will eventually
  // become page 0, and it will allow us (the kernel) to
  // write to it.
  __asm("\tlda\t#0x08");
  __asm("\tsta\t0xfe71");

  // Copy the file into memory but at page 1 not page 0!
  sptr= (char *)USERCODE + PAGESIZE;

  // Read the first 8190 (USERDATA - USERCODE) bytes into this page.
  // Then map in the usual frame, frame 1, into page 1
  i= sys_read(fd, sptr, USERDATA - USERCODE);
  __asm("\tlda\t#0x01");
  __asm("\tsta\t0xfe71");

  // Now read the rest of the file into memory. I'm pointing
  // sptr at USERDATA just to ensure we won't write on the
  // kernel data accidentally.
  sptr= (char *)USERDATA;
  while ((i = sys_read(fd, sptr, BSIZE)) > 0) sptr += BSIZE;
  sys_close(fd);

  // Close any open file descriptors
  for (fd = 0; fd < NFILE; fd++)
    sys_close(fd);

  // Now open fds 0, 1 and 2 to the console
  fd = sys_open(ttystr, O_RDONLY);
// cprintf("stdin %d\n", fd);
  fd = sys_open(ttystr, O_WRONLY);
// cprintf("stdout %d\n", fd);
  fd = sys_open(ttystr, O_WRONLY);
// cprintf("stderr %d\n", fd);

  // Start the code running with destbuf as the stack pointer value
  // and the arg count.
  jmptouser(argc, destbuf);
}

// Exit the currently-running program. Essentially
// this means spawn()ing "/bin/sh".
void sys_exit(int status) {
  
  // Spawn a /bin/sh
  sys_spawn(1, initargv);

  // The spawn failed but we can't return!
  panic("exit");
}
