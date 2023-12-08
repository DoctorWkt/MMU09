// Process details. Some of this from xv6.

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  enum procstate state;        	// Process state
  int pid;                     	// Process ID
  struct proc *parent;         	// Parent process
  char frame[NPAGES];		// Page table: eight frames
  void *chan;                  	// If non-zero, sleeping on chan
  int usersp;                	// Saved stack pointer
  int killed;                  	// If non-zero, have been killed
  int exitstatus;              	// Exit value, suitable for wait()
  struct file *ofile[NOFILE];  	// Open files
  struct inode *cwd;           	// Current directory
  char name[16];               	// Process name (debugging)
};
