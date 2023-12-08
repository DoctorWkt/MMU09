#ifndef ROMCALLS_H
#define ROMCALLS_H

// cprintf.c
#ifndef CPRINTF_REDEFINED
void cprintf(char *fmt, ...);
#endif

// sysfile.c
int sys_dup(int fd);
int sys_read(int fd, char *p, int n);
int sys_write(int fd, char *p, int n);
int sys_close(int fd);
int sys_fstat(int fd, struct xvstat *st);
int sys_link(char *old, char *new);
int sys_unlink(char *path);
int sys_open(char *path, int omode);
int sys_mkdir(char *path);
int sys_chdir(char *path);

void romputc(int ch);
int romgetputc(void);
void _exit(int status);
void spawn();
int _tcattr(int how, int newval);

// _tcattr: if how==1, set terminal with newval as follows

#define TC_ECHO		0x01	// Echo input characters

#endif
