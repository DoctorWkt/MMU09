// Code for the um6809 simulator that deals with
// the underlying operating system.
// (c) 2022, Warren Toomey, GPL3

#include "6809.h"

// SWI2 is now the interface to the
// system calls on the underlying system
void swi2 (void)
{
  int i;
  switch(A) {
    case 0: exit(0);
    case 1: i= (memory[S+2]<<8) + memory[S+3];                  // printint()
            printf("%d\n", i);
            break;
    default: fprintf(stderr, "Unknown SWI2 syscall %d\n", A); exit(1);
  }
}
