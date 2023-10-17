#define NOFILE       16		// open files per process
#define NFILE        16		// open files per system
#define NINODE       10		// maximum number of active i-nodes
#define MAXOPBLOCKS   6		// max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS)	// max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS)	// size of disk block cache
#define FSSIZE       1000	// size of file system in blocks
