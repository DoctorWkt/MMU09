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
// When the I/O area is enabled, the 256 bytes below the top ROM
// provide access to the I/O devices. This occurs when
// an SWI instruction occurs, or when the CPU takes an interrupt.
//
// When the 32K ROM is enabled, the top 32K of the address space is
// ROM except for the 256 byte I/O area.

#define PAGESIZE 8192
#define NUMFRAMES  64
#define NUMPAGES    8

// Static Variables
static int io_active[4];		// In kernel mode with I/O active?
static int rom_mapped[4];		// Is the 32K ROM mapped in?
static int io_idx= 0;			// Index into these arrays

struct pagetable {			// The page table
  UINT8 *frame;				// Pointer to the mapped frame
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

  // Put us into kernel mode and activate the I/O area.
  // Keep the 32K ROM deactivated.
  // Point the pages to the first eight frames. Mark them as valid.
  io_idx = 0;
  io_active[0] = 1;
  rom_mapped[0] = 0;
  for (int i=0; i < NUMPAGES; i++) {
    pte[i].frame= frame[i];
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
    fprintf(stderr, "bad addr 0x%04x PC 0x%04x in memory()\n",
	addr, get_pc()); exit(1);
  }

#if 0
  pagenum= addr >> 13;
  printf("%d: %X %X\n", pte[pagenum].pteval,
	(addr >> 8) & 0xff, addr & 0xff);
#endif

  // Access to the top 256 bytes always comes from ROM.
  if (addr >= 0xff00) {
    return(ROM[addr & 0x7fff]);
  }

  // Is the I/O area active?
  if (io_active[io_idx] && addr >= 0xfe00) {
      switch (addr) {
        case 0xfe10:
          // Read a character from the keyboard.
          return(kbread());
        case 0xfe30:
          // Read data from the CH375
          return(read_ch375_data());
        case 0xfe70:
        case 0xfe71:
        case 0xfe72:
        case 0xfe73:
        case 0xfe74:
        case 0xfe75:
        case 0xfe76:
        case 0xfe77:
	  // The real hardware can't read the PTEs but we can!
	  pagenum= addr & (NUMPAGES-1);
	  val= pte[pagenum].pteval;
	  return(val);
	default:
	  fprintf(stderr, "Unknown I/O location read 0x%04x PC 0x%04x\n",
	addr, get_pc());
	  return(0);
      }
  }

  // Top half of memory is ROM if the 32K ROM is mapped in
  if (rom_mapped[io_idx] && addr >= 0x8000) {
    return(ROM[addr & 0x7fff]);
  }

  // We are either in user-mode or accessing
  // RAM in kernel mode. Get the page number
  // and the offset on the page.
  pagenum= addr >> 13;	
  offset= addr & (PAGESIZE-1);

  // Get the byte from the page
  val= pte[pagenum].frame[offset];

  // If the page is marked invalid, XXX TO FIX
  if (pte[pagenum].pteval & 0x80) {
    fprintf(stderr, "invalid page read, addr 0x%04x PC 0x%04x\n", addr, get_pc());
  }

  return(val);
}

// Write a byte to virtual memory
void set_memory(unsigned addr, UINT8 data) {
  int pagenum, offset;
  int framenum;

  if (addr > 0xffff) {
    fprintf(stderr, "bad addr 0x%04x PC 0x%04x in set_memory()\n",
	addr, get_pc()); exit(1);
  }

#if 0
  pagenum= addr >> 13;
  printf("%d: %X %X\n", pte[pagenum].pteval,
	(addr >> 8) & 0xff, addr & 0xff);
#endif

  // Can't access the top 256 bytes as it is ROM.
  if (addr >= 0xff00) {
    fprintf(stderr, "ROM write 1 at 0x%04x PC 0x%04x in set_memory()\n",
	addr, get_pc());
    return;
  }

  // Is I/O active?
  if (io_active[io_idx] && addr >= 0xfe00) {
      switch (addr) {
        case 0xfe20:
          // Write a character to the UART, i.e. stdout
          putchar(data);
          fflush(stdout);
          return;
        case 0xfe40:
          // Write a data byte to the CH375.
          // If it returns true, do an FIRQ
          if (recv_ch375_data(data)) firq();
          return;
        case 0xfe41:
          // Write a command byte to the CH375.
          // If it returns true, do an FIRQ
          if (recv_ch375_cmd(data)) firq();
          return;
        case 0xfe50:
 	  // Disable the 32K ROM
// printf("rom unmapped\n");
	  rom_mapped[io_idx]= 0;
          return;
        case 0xfe51:
 	  // Enable the 32K ROM
// printf("rom mapped\n");
	  rom_mapped[io_idx]= 1;
          return;
        case 0xfe60:
// printf("I/O disabled\n");
	  // Go to user mode and map out the I/O area and 32K ROM.
	  // Also go back to io_idx zero.
	  io_idx= 0;
	  io_active[0]= 0;
	  rom_mapped[0]= 0;
	  return;
        case 0xfe70:
        case 0xfe71:
        case 0xfe72:
        case 0xfe73:
        case 0xfe74:
        case 0xfe75:
        case 0xfe76:
        case 0xfe77:
	  // Update a page table entry
// printf("Setting pte%d to 0x%x\n", addr & (NUMPAGES-1), data);
	  pagenum= addr & (NUMPAGES-1);
	  framenum= data & (NUMFRAMES-1);
	  pte[pagenum].frame= frame[framenum];
	  pte[pagenum].pteval= data;
	  return;
        case 0xfe80:
	  // Drop back to a previous I/O and 32K ROM mapping
	  io_idx--;
// printf("Moved io_idx down to %d: %d %d\n",
//	io_idx, io_active[io_idx], rom_mapped[io_idx]);
  	  if (io_idx<0) {
    	    fprintf(stderr, "Too many unstacked interrupts!\n"); exit(1);
  	  }
	  return;
	default:
	  fprintf(stderr, "Unknown I/O location write 0x%04x PC 0x%04x\n",
		addr, get_pc());
	  return;
      }
  }
   
  // Top half of memory is ROM
  if (rom_mapped[io_idx] && addr >= 0x8000) {
    fprintf(stderr, "ROM write 2 at 0x%04x PC 0x%04x SP 0x%04x in set_memory()\n",
	addr, get_pc(), get_s());
    return;
  }

  // We are either in user-mode or accessing
  // RAM in kernel mode. Get the page number
  // and the offset on the page.
  pagenum= addr >> 13;	
  offset= addr & (PAGESIZE-1);

  // Write the byte to the page
  pte[pagenum].frame[offset]= data;

  // If the page is marked invalid, XXX TO FIX
  if (pte[pagenum].pteval & 0x80) {
    fprintf(stderr, "invalid page write, addr 0x%04x PC 0x%04x\n",
	addr, get_pc());
    return;
  }
}

// Set up intial memory contents
void set_initial_memory(unsigned addr, UINT8 data) {
  int pagenum, offset;

  if (addr > 0xffff) {
    fprintf(stderr, "bad addr 0x%04x PC 0x%04x in set_initial_memory()\n",
	addr, get_pc()); exit(1);
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

// Set kernel mode and make the I/O area visible
void set_io_active(void) {
  // Move up to the next position, so we remember the previous settings
  io_idx++;
  if (io_idx==4) {
    fprintf(stderr, "Too many stacked interrupts!\n"); exit(1);
  }
  io_active[io_idx] = 1;
  rom_mapped[io_idx]= 0;
// printf("I/O mapped, ROM unmapped\n");
// printf("Moved io_idx up to %d: %d %d\n",
//	io_idx, io_active[io_idx], rom_mapped[io_idx]);
}
