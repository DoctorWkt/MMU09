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

extern struct watchpoint *watchhead;
int total = 0;
char *exename;
char *ch375file= "unknown";

// Debug output file
FILE *debugout= NULL;

// Add a watchpoint to the linked list
static void add_watchpoint(int addr) {
  struct watchpoint *this;
  this=(struct watchpoint *)malloc(sizeof(struct watchpoint));
  if (this==NULL) {
    fprintf(stderr, "malloc failed in add_watchpoint\n"); exit(1); 
  }
  this->addr= addr;

  // Prepend to the linked list
  if (watchhead==NULL)
    this->next= NULL;
  else
    this->next= watchhead;
  watchhead= this;
}

static void usage (void)
{
  printf("Usage: %s <options> s19_filename\n",exename);
  printf("Options are:\n");
  printf("-m        - start in the monitor\n");
  printf("-x        - randomise memory contents\n");
  printf("-b   addr - set a breakpoint in hex and run until that address\n");
  printf("-s   addr - set stack start address in hex\n");
  printf("-a   addr - start address in hex (instead of reset vector)\n");
  printf("-i   name - use the named fs image for CH375 block operations\n");
  printf("-p   name - also load the named s19 image\n");
  printf("-d   name - write debug output to this file\n");
  printf("-w   addr - stop execution when there is a write to this address\n");
  exit (1);
}


int main (int argc, char *argv[])
{
  char *name;
  int opt;
  int breakpoint= -1;
  int randomise_mem=0;
  int start_in_monitor=0;
  int start_addr= -1;
  int start_stack= 0xC7FF;		// Nine-E V1. V2 will be $D7FF

  exename = argv[0];

  if (argc == 1) usage();

  init_memory();			// Set up the memory and mappings

  // Get the options
  while ((opt = getopt(argc, argv, "mxb:s:a:i:p:d:w:")) != -1) {
    switch(opt) {
      case 'm': start_in_monitor=1; break;
      case 'x': randomise_mem=1; break;
      case 'b': breakpoint= strtoul(optarg,NULL,16); break;
      case 's': start_stack= strtoul(optarg,NULL,16); break;
      case 'a': start_addr= strtoul(optarg,NULL,16); break;
      case 'i': ch375file= optarg; break;
      case 'p': if (load_s19(optarg)) usage(); break;
      case 'd': if ((debugout=fopen(optarg, "w"))==NULL) {
		  fprintf(stderr, "Unable to open %s\n", optarg); exit(1); 
		}
		break;
      case 'w': add_watchpoint(strtoul(optarg,NULL,16));
		break;
    }
  }

  // Randomise memory if required
  if (randomise_mem)
    randomise_memory();

  // Get the filename to load
  if (optind >= argc) usage();
  name= argv[optind];

  cpu_quit = 1;

  // Load the binary
  if(load_s19(name)) usage();

  monitor_init(start_in_monitor);

  // If there's a breakpoint, set it
  if (breakpoint != -1) {
    add_breakpoint(breakpoint);
    monitor_on = 0;
    do_break = 1;
  }

  cpu_reset(start_addr, start_stack);

  do
  {
    total += cpu_execute (6000000);
  } while (cpu_quit != 0);

  printf("6809 stopped after %d cycles\n",total);

  return 0;
}
