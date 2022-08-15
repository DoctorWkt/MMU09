// Code for the um6809 simulator that deals with
// the underlying operating system.
// (c) 2022, Warren Toomey, GPL3

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include "6809.h"
#include "oscode.h"

// SWI2 is now the interface to the
// system calls on the underlying system
void swi2 (void)
{
					// We use 16-bit variables
					// and then rely on the compiler
					// to widen/sign-extend them
  int16_t sarg1, sarg2, sarg3;
  u_int16_t uarg1;
  char *str1, *str2;
  int pipefd[2];			// for pipe(2)
  int retval;				// Return val for some syscalls
  time_t tim;

  unsigned int S= get_s();
  unsigned int A= get_a();

// STILL TO DO:
// exec(), fstat(),  sbrk()
// uptime(), ioctl()
  
  switch(A) {
    case S_EXIT:
	sarg1= GET_WD_ARG(2);
	exit(sarg1);

    case S_OPEN:
	str1= (char *)GET_PTR_ARG(2);
	sarg1= GET_WD_ARG(4);
	set_d( open(str1, sarg1) );
	break;

    case S_CLOSE:
	sarg1= GET_WD_ARG(2);
	set_d( close(sarg1) );
	break;

    case S_READ:
	sarg1= GET_WD_ARG(2);
	str1= (char *)GET_PTR_ARG(4);
	sarg3= GET_WD_ARG(6);
	set_d( read(sarg1, str1, sarg3) );
	break;

    case S_WRITE:
	sarg1= GET_WD_ARG(2);
	str1= (char *)GET_PTR_ARG(4);
	sarg3= GET_WD_ARG(6);
	set_d( write(sarg1, str1, sarg3) );
	break;

    case S_PRINTINT:
	sarg1= GET_WD_ARG(2);
	printf("%d\n", sarg1);
	break;

    case S_GETPID:
	set_d( getpid() );
	break;

    case S_CHDIR:
	str1= (char *)GET_PTR_ARG(2);
	set_d( chdir(str1) );
	break;

    case S_DUP:
	sarg1= GET_WD_ARG(2);
	set_d( dup(sarg1) );
	break;

    case S_FORK:
	set_d( fork() );
	break;

    case S_KILL:
	sarg1= GET_WD_ARG(2);
	sarg2= GET_WD_ARG(4);
	set_d( kill(sarg1, sarg2) );
	break;

    case S_LINK:
	str1= (char *)GET_PTR_ARG(2);
	str2= (char *)GET_PTR_ARG(4);
	set_d( link(str1, str2) );
	break;

    case S_MKDIR:
	str1= (char *)GET_PTR_ARG(2);
	sarg2= GET_WD_ARG(4);
	set_d( mkdir(str1, sarg2) );
	break;

    case S_MKNOD:
	str1= (char *)GET_PTR_ARG(2);
	sarg2= GET_WD_ARG(4);
	sarg3= GET_WD_ARG(6);
	set_d( mknod(str1, sarg2, sarg3) );
	break;

    case S_SLEEP:
	sarg1= GET_WD_ARG(2);
	set_d( sleep(sarg1) );
	break;

    case S_UNLINK:
	str1= (char *)GET_PTR_ARG(2);
	set_d( unlink(str1) );
	break;

    case S_LSEEK:
	sarg1= GET_WD_ARG(2);
	sarg2= GET_WD_ARG(4);
	sarg3= GET_WD_ARG(6);
	set_d( lseek(sarg1, sarg2, sarg3) );
	break;

    case S_FCHDIR:
	sarg1= GET_WD_ARG(2);
	set_d( fchdir(sarg1) );
	break;

    case S_PIPE:
        // Get the address of the int[2] array
	uarg1= GET_WD_ARG(2);
        // Do the pipe(2) syscall
        retval= pipe(pipefd);
        // Save the two file descriptors and return value
 	PUT_WD(uarg1, pipefd[0]);
 	PUT_WD(uarg1+2, pipefd[1]);
	set_d(retval);
	break;

    case S_TIME:
        // Get the address of the long
	uarg1= GET_WD_ARG(2);
	tim= time(NULL);
 	PUT_WD(uarg1, (tim >> 16));
 	PUT_WD(uarg1+2, tim);
	break;

    case S_WAIT:
        // Get the address of the int, then the value there
	uarg1= GET_WD_ARG(2);
	retval= GET_WD(uarg1);
	sarg1= wait(&retval);
	// Put the value back into the int
 	PUT_WD(uarg1, retval);
	set_d(sarg1);
	break;


    default: fprintf(stderr, "Unknown SWI2 syscall %d\n", A); exit(1);
  }
}


// Arrays of arguments and environment variables
char *Argv[MAX_ARGS], *Envp[MAX_ARGS];
int Argc, Envc;

/* For programs without an environment, we set the environment statically.
 * Eventually there will be code to get some environment variables
 */
static int default_envc = 4;
static char *default_envp[4] = {
    "PATH=/bin:/usr/bin:/usr/local/bin:.",
    "HOME=/",
    "TERM=vt100",
    "USER=root"
};


/*
 * C runtime startoff.	When a binary is loaded by the MMU09 kernel,
 * and before the JSR to main(), the kernel sets up the stack as follows:
 *
 *	_________________________________
 *	| (NULL)			| $FFFE/$FFFF: top of RAM
 *	|-------------------------------|
 *	|				|
 *	| environment strings		|
 *	|				|
 *	|-------------------------------|
 *	|				|
 *	| argument strings		|
 *	|				|
 *	|-------------------------------|
 *	| envv[envc] (NULL)		| end of environment vector tag, a 0
 *	|-------------------------------|
 *	| envv[envc-1]			| pointer to last environment string
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| envv[0]			| pointer to first environment string
 *	|-------------------------------|
 *	| argv[argc] (NULL)		| end of argument vector tag, a 0
 *	|-------------------------------|
 *	| argv[argc-1]			| pointer to last argument string
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| argv[0]			| pointer to first argument string
 *	|-------------------------------|
 *	| envp itself			| pointer to envp[0]
 *	|-------------------------------|
 *	| argv itself			| pointer to argv[0]
 *	|-------------------------------|
 * sp-> | argc				| number of arguments
 *	---------------------------------
 *
 */
static void set_arg_env (void)
{
  int i, posn, len;
  int envposn;
  int eposn[MAX_ARGS];
  int aposn[MAX_ARGS];

  /* Set default environment if there is none */
  if (Envp[0] == NULL) {
      Envc = default_envc;
      for (i = 0; i < Envc; i++)
	Envp[i] = default_envp[i];
  }

  /* Now build the arguments and pointers on the stack */
  /* Start the stack below $FF00 */

  posn = 0xFF00 - 2;
  PUT_WD (posn, 0);			/* Put a NULL on top of stack */

  for (i = Envc - 1; i != -1; i--) {	/* For each env string */
      len = strlen (Envp[i]) + 1;	/* get its length */
      posn -= len;
      memcpy (&memory[posn], Envp[i], (size_t) len);
      eposn[i] = posn;
  }

  for (i = Argc - 1; i != -1; i--) {	/* For each arg string */
      len = strlen (Argv[i]) + 1;	/* get its length */
      posn -= len;
      memcpy (&memory[posn], Argv[i], (size_t) len);
      aposn[i] = posn;
  }
  posn -= 2;
  PUT_WD (posn, 0);			/* Put a NULL at end of env array */

  for (i = Envc - 1; i != -1; i--) {	/* For each env string */
      posn -= 2;			/* put a pointer to the string */
      PUT_WD (posn, (u_int16_t) eposn[i]);
  }
  envposn= posn; posn -= 2;

  /* Put a NULL before arg ptrs */
  PUT_WD (posn, 0);

  for (i = Argc - 1; i != -1; i--) {		/* For each arg string */
      posn -= 2;
      PUT_WD (posn, (u_int16_t) aposn[i]);	/* put a ptr to the string */
  }
  posn -= 2;
  PUT_WD (posn, (u_int16_t) envposn);		/* Pointer to envp[0] */
  posn -= 2;
  PUT_WD (posn, (u_int16_t) (posn+2));		/* Pointer to argv[0] */
  posn -= 2;
  PUT_WD (posn, (u_int16_t) Argc);		/* Save the count of args */
  set_s(posn);					/* and initialise the SP */
  set_u(posn);					/* and initialise the SP */
}

void set_arglist(int argc, char **argv) {
    int i;

    /* Prepare the argument list for the emulated environment */
    Argc = argc;
    Envp[0] = NULL;
    for (i = 0; i < argc; i++)
        Argv[i] = argv[i];
    set_arg_env();
}
