struct file {
  enum { FD_NONE, FD_CONSOLE, FD_PIPE, FD_INODE } type;
  Int ref;			// reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  xvoff_t off;
};


// in-memory copy of an inode
struct inode {
  xvino_t inum;			// Inode number
  Int ref;			// Reference count
  Int valid;			// inode has been read from disk?

  short type;			// copy of disk inode
  short nlink;
  xvoff_t size;
  xvblk_t addrs[NDIRECT + 1];
};

#define CONSOLE 1

#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

