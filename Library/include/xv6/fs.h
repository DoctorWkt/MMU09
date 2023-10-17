// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO 1		// root i-number
#define BSIZE 512		// block size
#define bzero xv6bzero		// Prevent conflict with Unix string function

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  xvblk_t size;			// Size of file system image (blocks)
  xvblk_t nblocks;		// Number of data blocks
  xvino_t ninodes;		// Number of inodes.
  xvblk_t nlog;			// Number of log blocks
  xvblk_t logstart;		// Block number of first log block
  xvblk_t inodestart;		// Block number of first inode block
  xvblk_t bmapstart;		// Block number of first free map block
};

#define NDIRECT 27
#define NINDIRECT (BSIZE / sizeof(xvoff_t))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;			// File type
  short nlink;			// Number of links to inode in file system
  xvoff_t size;			// Size of file (bytes)
  xvblk_t addrs[NDIRECT + 1];	// Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) (b/BPB + sb.bmapstart)

// Directory is a file containing a sequence of xvdirent structures.
#define DIRSIZ 14

struct xvdirent {
  xvino_t inum;
  char name[DIRSIZ];
};
