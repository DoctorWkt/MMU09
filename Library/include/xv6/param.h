#define NOFILE       16		// open files per process
#define NFILE        16		// open files per system
#define NPROC	     16		// number of processes
#define PGSIZE	   8192		// Pages are 8K in size
#define NPAGES	      8		// Eight 8K pages per process
#define NFRAMES	     64		// Sixty four 8K page frames
#define NINODE       10		// maximum number of active i-nodes
#define MAXOPBLOCKS   6		// max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS)	// max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS)	// size of disk block cache
#define FSSIZE       1000	// size of file system in blocks
