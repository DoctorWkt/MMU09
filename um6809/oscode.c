// Code for the um6809 simulator that deals with
// the underlying operating system.
// (c) 2022, Warren Toomey, GPL3

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "6809.h"
#include "oscode.h"

// SWI2 is now the interface to the
// system calls on the underlying system
void swi2 (void)
{
					// We use 16-bit variables
					// and then rely on the compiler
					// to widen/sign-extend them
  int16_t sarg1, sarg3;
  unsigned int S= get_s();
  unsigned int A= get_a();

  char *str1;
  
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

    default: fprintf(stderr, "Unknown SWI2 syscall %d\n", A); exit(1);
  }
}
