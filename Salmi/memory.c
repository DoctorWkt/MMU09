// Code to deal with virtual memory.
// (c) 2023 Warren Toomey, GPL3.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "6809.h"

// We have 64 page frames of 8K each, making 512K of RAM in total.
// These are mapped to eight 8K pages in user mode, except that
// the top 256 bytes are mapped to the top of ROM.
//
// However, in kernel mode, the top 32K of the address space is
// ROM, except a 256 byte "window" at $FFEx to map I/O devices.
// In kernel mode, there are four 8K pages in the bottom 32K.

#define PAGESIZE 8192
#define NUMFRAMES  64
#define NUMPAGES    8

// Static Variables
static int kernelmode = 1;		// Are we in kernel mode?

struct pagetable {			// The page table
  UINT8 *frame;				// Pointer to the mapped frame
  int invalid;				// If true, page is invalid
  int pteval;				// Real hardware pte value
} pte[NUMPAGES];				

UINT8 *frame[NUMFRAMES];		// The 64 8K frames

UINT8 ROM[PAGESIZE * (NUMPAGES/2)];	// The 32K of ROM

// Set up the initial memory map
void init_memory(void) {

  // Make the page frames
  for (int i=0; i<NUMFRAMES; i++) {
    frame[i]= (UINT8 *)malloc(PAGESIZE);
    if (frame[i]==NULL) {
      fprintf(stderr, "malloc fail in init_memory()\n"); exit(1);
    }
    memset (frame[i], 0, PAGESIZE);
  }

  // Put us into kernel mode. Point the pages to the
  // first eight frames. Mark them as valid.
  kernelmode = 1;
  for (int i=0; i < NUMPAGES; i++) {
    pte[i].frame= frame[i];
    pte[i].invalid= 0;
    pte[i].pteval= i;
  }
}

// Randomise the contents of RAM in all frames
void randomise_memory(void) {
  for (int i=0; i < NUMFRAMES; i++)
    for (int j=0; j < PAGESIZE; j++)
      frame[i][j]= (UINT8)rand();
}

// Read a byte from virtual memory
UINT8 memory(unsigned addr) {
  int pagenum, offset;
  UINT8 val;

  if (addr > 0xffff) {
    fprintf(stderr, "bad addr %d in memory()\n", addr); exit(1);
  }

  // Access to the top page always comes from ROM
  if (addr >= 0xfff0) {
    return(ROM[addr & 0x7fff]);
  }

  // Are we in kernel mode?
  if (kernelmode) {
    // Is it an I/O access? XXX to fix
    if (addr >= 0xfe00) {
      switch (addr) {
        case 0xfe20:
          // Read a character from the keyboard.
          return(kbread());
        case 0xfe60:
          // Read data from the CH375
          return(read_ch375_data());
        case 0xfec0:
        case 0xfec1:
        case 0xfec2:
        case 0xfec3:
        case 0xfec4:
        case 0xfec5:
        case 0xfec6:
        case 0xfec7:
	  // The real hardware can't read the PTEs but we can!
	  pagenum= addr & (NUMPAGES-1);
	  val= pte[pagenum].pteval;
	  return(val);
	default:
	  fprintf(stderr, "Unknown I/O location read 0x%04x\n", addr);
	  return(0);
      }
    }

    // Top half of memory is ROM
    if (addr >= 0x8000) {
      return(ROM[addr & 0x7fff]);
    }
  }

  // We are either in user-mode or accessing
  // RAM in kernel mode. Get the page number
  // and the offset on the page.
  pagenum= addr >> 13;	
  offset= addr & (PAGESIZE-1);

  // Get the byte from the page
  val= pte[pagenum].frame[offset];

  // If the page is marked invalid, XXX TO FIX
  if (pte[pagenum].invalid) {
    fprintf(stderr, "invalid page read, addr 0x%04x\n", addr);
  }

  return(val);
}

// Write a byte to virtual memory
void set_memory(unsigned addr, UINT8 data) {
  int pagenum, offset;
  int framenum;

  if (addr > 0xffff) {
    fprintf(stderr, "bad addr %d in set_memory()\n", addr); exit(1);
  }

  // Can't access the top page as it is ROM
  if (addr >= 0xfff0) {
    fprintf(stderr, "ROM write at 0x%04x in set_memory()\n", addr);
    return;
  }

  // Are we in kernel mode?
  if (kernelmode) {
    // Is it an I/O access? XXX to fix
    if (addr >= 0xfe00) {
      switch (addr) {
        case 0xfe40:
          // Write a character to the UART, i.e. stdout
          putchar(data);
          fflush(stdout);
          return;
        case 0xfe80:
          // Write a data byte to the CH375.
          // If it returns true, do an FIRQ
          if (recv_ch375_data(data)) firq();
          return;
        case 0xfe81:
          // Write a command byte to the CH375.
          // If it returns true, do an FIRQ
          if (recv_ch375_cmd(data)) firq();
          return;
        case 0xfee0:
	  // Go to user mode
	  kernelmode= 0;
	  return;
        case 0xfec0:
        case 0xfec1:
        case 0xfec2:
        case 0xfec3:
        case 0xfec4:
        case 0xfec5:
        case 0xfec6:
        case 0xfec7:
	  // Update a page table entry
	  pagenum= addr & (NUMPAGES-1);
	  framenum= data & (NUMFRAMES-1);
	  pte[pagenum].frame= frame[framenum];
	  pte[pagenum].invalid= (data & 0x80) ? 1 : 0;
	  pte[pagenum].pteval= data;
	  break;
	default:
	  fprintf(stderr, "Unknown I/O location write 0x%04x\n", addr);
	  return;
      }
    }

    // Top half of memory is ROM
    if (addr >= 0x8000) {
      fprintf(stderr, "ROM write at 0x%04x in set_memory()\n", addr);
      return;
    }
  }

  // We are either in user-mode or accessing
  // RAM in kernel mode. Get the page number
  // and the offset on the page.
  pagenum= addr >> 13;	
  offset= addr & (PAGESIZE-1);

  // Write the byte to the page
  pte[pagenum].frame[offset]= data;

  // If the page is marked invalid, XXX TO FIX
  if (pte[pagenum].invalid) {
    fprintf(stderr, "invalid page write, addr 0x%04x\n", addr);
    return;
  }
}

// Set up intial memory contents
void set_initial_memory(unsigned addr, UINT8 data) {
  int pagenum, offset;

  if (addr > 0xffff) {
    fprintf(stderr, "bad addr %d in set_initial_memory()\n", addr); exit(1);
  }

  // Write to ROM if the top half of memory
  if (addr >= 0x8000) {
      ROM[addr & 0x7fff]= data; return;
  }

  // Otherwise place data in RAM
  pagenum= addr >> 13;	
  offset= addr & (PAGESIZE-1);
  pte[pagenum].frame[offset]= data;
}

// Set kernel mode
void set_kernelmode(void) {
  kernelmode = 1;
}
