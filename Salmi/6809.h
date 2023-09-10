/*
    6809.h   	-	header for monitor and simulator
    Copyright (C) 2001  Arto Salmi

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


#ifndef M6809_H
#define M6809_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

typedef unsigned char UINT8;
typedef signed char INT8;

typedef unsigned short UINT16;
typedef signed short INT16;

typedef unsigned int UINT32;
typedef signed int INT32;

#define E_FLAG 0x80
#define F_FLAG 0x40
#define H_FLAG 0x20
#define I_FLAG 0x10
#define N_FLAG 0x08
#define Z_FLAG 0x04
#define V_FLAG 0x02
#define C_FLAG 0x01

// Memory watchpoints are stored in a linked list of this struct
struct watchpoint {
  int addr;
  struct watchpoint *next;
};

/* 6809.c */
extern int cpu_quit;
extern int cpu_execute (int);
extern void cpu_reset (int, int);
extern void shutdown_fs(void);

extern unsigned kbread(void);
extern void firq (void);

extern unsigned get_a  (void);
extern unsigned get_b  (void);
extern unsigned get_cc (void);
extern unsigned get_dp (void);
extern unsigned get_x  (void);
extern unsigned get_y  (void);
extern unsigned get_s  (void);
extern unsigned get_u  (void);
extern unsigned get_pc (void);
extern unsigned get_d  (void);
extern void set_a  (unsigned);
extern void set_b  (unsigned);
extern void set_cc (unsigned);
extern void set_dp (unsigned);
extern void set_x  (unsigned);
extern void set_y  (unsigned);
extern void set_s  (unsigned);
extern void set_u  (unsigned);
extern void set_pc (unsigned);
extern void set_d  (unsigned);
extern void WRMEM16 (unsigned addr, unsigned data);

/* memory.c */
extern void init_memory(void);
extern void randomise_memory(void);
extern UINT8 memory(unsigned addr);
extern void set_memory(unsigned addr, UINT8 data);
extern void set_initial_memory(unsigned addr, UINT8 data);
void set_kernelmode(void);

/* monitor.c */
extern int monitor_on;
extern int do_break;
extern int check_break (unsigned);
extern void monitor_init (int); 
extern int monitor6809 (void);
extern int dasm (char *, int);
extern void add_breakpoint (int break_pc);

extern int load_hex (char *);
extern int load_s19 (char *);
extern int load_bin (char *,int);

/* ch375.c */
extern unsigned char read_ch375_data(void);
extern unsigned recv_ch375_cmd(unsigned char cmd);
extern unsigned recv_ch375_data(unsigned char data);

/* uart.c */
extern void reset_terminal_mode(void);
extern void save_old_terminal_mode(void);
extern int kbhit(void);
extern unsigned kbread(void);
extern int ttySetCbreak(void);

#endif /* M6809_H */
