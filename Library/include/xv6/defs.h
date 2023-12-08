struct buf;
struct file;
struct inode;
struct xvstat;
struct superblock;
struct pipe;

// XXX
void panic(char *);

// blk.c
void blkinit(void);
void blkrw(struct buf *b);

// bio.c
void binit(void);
struct buf *bread(xvblk_t);
void brelse(struct buf *);
void bwrite(struct buf *);

// cprintf.c
#ifndef CPRINTF_REDEFINED
void cprintf(char *fmt, ...);
#endif

// file.c
struct file *filealloc(void);
void fileclose(struct file *);
struct file *filedup(struct file *);
void fileinit(void);
Int fileread(struct file *, char *, xvoff_t n);
Int filestat(struct file *, struct xvstat *);
xvoff_t filewrite(struct file *, char *, xvoff_t n);

// fs.c
void readsb(struct superblock *sb);
Int dirlink(struct inode *, char *, xvino_t);
struct inode *dirlookup(struct inode *, char *, xvoff_t *);
struct inode *ialloc(short);
struct inode *idup(struct inode *);
void iinit();
void ilock(struct inode *);
void iput(struct inode *);
// void itrunc(struct inode *ip);	// Temp change
void iunlock(struct inode *);
void iunlockput(struct inode *);
void iupdate(struct inode *);
Int namecmp(const char *, const char *);
struct inode *namei(char *);
struct inode *nameiparent(char *, char *);
xvoff_t readi(struct inode *, char *, xvoff_t, xvoff_t);
void stati(struct inode *, struct xvstat *);
xvoff_t writei(struct inode *, char *, xvoff_t, xvoff_t);
extern struct inode *cwd;

// log.c
void initlog(void);
void log_write(struct buf *);
void begin_op();
void end_op();

// romfuncs.s
void rommemcpy(Int, void *, void *);
void romstrncpy(Int, void *, void *);
int romstrlen(char *);
void romputc(int ch);
int romgetputc(void);
int ch375init(void);
int readblock(unsigned char *buf, long lba);
int writeblock(unsigned char *buf, long lba);
void jmptouser(Int memsize, Int argc, char *destbuf);
void set_errno(Int);

// syscall.c
Int argint(Int, Int *);
Int argptr(Int, char **, Int);
Int argstr(Int, char **);
Int fetchint(Uint, Int *);
Int fetchstr(Uint, char **);

// sysfile.c
Int sys_dup(Int fd);
Int kread(Int fd, char *p, Int n);
Int sys_read(Int fd, long d1, long d2, long d3, int d4, char *p, Int n);
Int kwrite(Int fd, char *p, Int n);
Int sys_write(Int fd, long d1, long d2, long d3, int d4, char *p, Int n);
Int sys_close(Int fd);
Int sys_fstat(Int fd, long d1, long d2, long d3, int d4, struct xvstat *st);
Int sys_link(char *old, long d1, long d2, long d3, int d4, char *new);
Int sys_unlink(char *path);
Int kopen(char *path, Int omode);
Int sys_open(char *path, long d1, long d2, long d3, int d4, Int omode);
Int sys_mkdir(char *path);
Int sys_chdir(char *path);
int sys_pipe(int *fd);
void sys_init(void);		// For now!

// exit.c
extern char userbuf[512];

/* proc.c */
void copypage(char *from, char toframe);
int fork1(void);
void exec(int argc, char *argv[]);
void sched(void);
void sched1(void);
void sys_exit(int exitvalue);
void sleepchan(void *chan);
int sys_wait(int *statusptr);
void wakeup(void *chan);
void procinit(void);
void sys_exec(int argc, long d0, long d1, long d2, int d3, char *argv[]);
int sys_getpid(void);
int kkill(int pid);
int sys_kill(int pid);
extern struct proc *curproc;

/* pipe.c */
void pipeinit(void);
int pipealloc(struct file **f0, struct file **f1);
void pipeclose(struct pipe *p, int writable);
int pipewrite(struct pipe *p, char *addr, int n);
int piperead(struct pipe *p, char *addr, int n);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

#ifdef __linux__
#define cprintf printf
#endif
