/*
    main.c   	-	main program for 6809 simulator
    Copyright (C) 2001  Arto Salmi
                        Joze Fabcic

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "6809.h"

enum { HEX, S19, BIN };

int total = 0;

char *exename;

static void usage (void)
{
  printf("Usage: %s <options> filename [arguments ...]\n",exename);
  printf("Options are:\n");
  printf("-hex	- load intel hex file\n");
  printf("-s19	- load motorola s record file\n");
  printf("-bin	- load binary file\n");
  printf("-s addr - specify binary load address hexadecimal (default 0)\n");
  printf("-b addr - set a breakpoint in hex and run until that address\n");
  printf("default format is motorola s record\n");

  if (memory != NULL) free(memory);
  exit (1);
}


int main (int argc, char **argv)
{
  int type = S19;
  int off  = 0;
  int breakpoint= -1;

  memory = (UINT8 *)malloc(64 * 1024);		// Allocate 6K of RAM
  if (memory == NULL) usage();
  memset (memory, 0, 0x10000);

  cpu_reset();

  exename = argv[0];
  if ((argc < 2) || !strcmp(argv[1], "-help") ||
     !strcmp(argv[1], "--help")) usage();

  // Skip past the "um6809" argument
  argc--; argv++;
  
  while (1) {
    if (argc < 1) usage();

    if (!strcmp(argv[0], "-hex")) {
      type = HEX; argc--; argv++; continue;
    }

    if (!strcmp(argv[0], "-s19")) {
      type = S19; argc--; argv++; continue;
    }

    if (!strcmp(argv[0], "-bin")) {
      type = BIN; argc--; argv++; continue;
    }

    if (!strcmp(argv[0], "-s")) {
      argc--; argv++;
      if (argc<1) usage();
      off  = strtoul(argv[0],NULL,16); type = BIN;
      argc--; argv++; continue;
    }

    if (!strcmp(argv[0], "-b")) {
      argc--; argv++;
      if (argc<1) usage();
      breakpoint= strtoul(argv[0],NULL,16);
      argc--; argv++; continue;
    }

    break;
  }

  // Set up the argument and environment list
  set_arglist(argc, argv);
 
  cpu_quit = 1;

  switch (type)
  {
    case HEX: if(load_hex(argv[0])) usage(); break;
    case S19: if(load_s19(argv[0])) usage(); break;
    case BIN: if(load_bin(argv[0],off&0xffff)) usage(); break;
  }  

  monitor_init();
  monitor_on = 0;	// Don't start the monitor unless a breakpoint

  // If there's a breakpoint, set it
  if (breakpoint != -1) {
    add_breakpoint(breakpoint);
    do_break = 1;
  }

  do
  {
    total += cpu_execute (60);
  } while (cpu_quit != 0);

  printf("6809 stopped after %d cycles\n",total);

  free(memory);

  return 0;
}
