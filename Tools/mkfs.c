// mkfs from xv6 with modifications by Warren Toomey
// (c) 2023, BSD license

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <assert.h>

#include "../Library/include/xv6/types.h"
#include "../Library/include/xv6/fs.h"
#include "../Library/include/xv6/stat.h"
#include "../Library/include/xv6/param.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

#define NINODES 200

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

int nbitmap = FSSIZE / (BSIZE * 8) + 1;
int ninodeblocks = NINODES / IPB + 1;
int nlog = LOGSIZE;
int nmeta;			// Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks;			// Number of data blocks

int fsfd;
struct superblock sb;
struct superblock nativesb;     // sb but in our endian
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;


void balloc(int);
void wsect(uint, void *);
void winode(uint, struct dinode *);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);
void dappend(int dirino, char *name, int fileino);
void add_directory(int dirino, char *localdir);
int makdir(int dirino, char *newdir, struct stat *sb);

#ifdef BIG_ENDIAN_6809
ushort xshort(ushort x) {
  ushort y;
  uchar *a = (uchar *) & y;
  a[1] = x;
  a[0] = x >> 8;
  return y;
}

uint xint(uint x) {
  uint y;
  uchar *a = (uchar *) & y;
  a[3] = x;
  a[2] = x >> 8;
  a[1] = x >> 16;
  a[0] = x >> 24;
  return y;
}
#else
// Conversions to little-endian byte order
ushort xshort(ushort x) {
  ushort y;
  uchar *a = (uchar *) & y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint xint(uint x) {
  uint y;
  uchar *a = (uchar *) & y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}
#endif
int main(int argc, char *argv[]) {
  int i;
  xvoff_t off;
  xvino_t rootino;
  char buf[BSIZE];
  struct dinode din;

  static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

  if (argc != 3) {
    fprintf(stderr, "Usage: mkfs fs.img basedir\n");
    exit(1);
  }

  assert((BSIZE % sizeof(struct dinode)) == 0);
  assert((BSIZE % sizeof(struct xvdirent)) == 0);

  // Open the filesystem image file
  fsfd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fsfd < 0) {
    perror(argv[1]);
    exit(1);
  }
  // 1 fs block = 1 disk sector
  // Number of meta blocks: boot block, superblock, log blocks,
  // i-node blocks and the free bitmap blocks
  nmeta = 2 + nlog + ninodeblocks + nbitmap;
  // Now work out how many free blocks are left
  nblocks = FSSIZE - nmeta;

  // Set up the superblock, and a native-endian version
  sb.size = xshort(FSSIZE);
  sb.nblocks = xshort(nblocks);
  sb.ninodes = xshort(NINODES);
  sb.nlog = xshort(nlog);
  sb.logstart = xshort(2);
  sb.inodestart = xshort(2 + nlog);
  sb.bmapstart = xshort(2 + nlog + ninodeblocks);
  nativesb.size = FSSIZE;
  nativesb.nblocks = nblocks;
  nativesb.ninodes = NINODES;
  nativesb.nlog = nlog;
  nativesb.logstart = 2;
  nativesb.inodestart = 2 + nlog;
  nativesb.bmapstart = 2 + nlog + ninodeblocks;

  printf
    ("nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
     nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

  freeblock = nmeta;		// The first free block that we can allocate

  // Fill the filesystem with zero'ed blocks
  for (i = 0; i < FSSIZE; i++)
    wsect(i, zeroes);

  // Copy the superblock struct into a zero'ed buf
  // and write it out as block 1
  memset(buf, 0, sizeof(buf));
  memmove(buf, &sb, sizeof(sb));
  wsect(1, buf);

  // Grab an i-node for the root directory
  rootino = ialloc(T_DIR);	// Epoch mtime for now
  assert(rootino == ROOTINO);

  // Set up the directory entry for . and add it to the root dir
  // Set up the directory entry for .. and add it to the root dir
  dappend(rootino, ".", rootino);
  dappend(rootino, "..", rootino);

  // Add the contents of the command-line directory to the root dir
  add_directory(rootino, argv[2]);

  // Fix the size of the root inode dir
  rinode(rootino, &din);
  off = xint(din.size);
  off = ((off / BSIZE) + 1) * BSIZE;
  din.size = xint(off);
  winode(rootino, &din);

  // Mark the in-use blocks in the free block list
  balloc(freeblock);

  exit(0);
}

// Write a sector to the image
void wsect(uint sec, void *buf) {
// printf("In wsect block 0x%x offset 0x%x\n", sec, sec * 512);
  if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
    perror("lseek");
    exit(1);
  }
  if (write(fsfd, buf, BSIZE) != BSIZE) {
    perror("write");
    exit(1);
  }
}

// Write an i-node to the image
void winode(uint inum, struct dinode *ip) {
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, nativesb);
  rsect(bn, buf);
  dip = ((struct dinode *) buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}

// Read an i-node from the image
void rinode(uint inum, struct dinode *ip) {
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, nativesb);
  rsect(bn, buf);
  dip = ((struct dinode *) buf) + (inum % IPB);
  *ip = *dip;
}

// Read a sector from the image
void rsect(uint sec, void *buf) {
  if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
    perror("lseek");
    exit(1);
  }
  if (read(fsfd, buf, BSIZE) != BSIZE) {
    perror("read");
    exit(1);
  }
}

// Allocate an i-node
uint ialloc(ushort type) {
  uint inum = freeinode++;
  struct dinode din;

  assert(freeinode < NINODES);
  memset(&din, 0, sizeof(din));
  din.type = xshort(type);
  din.nlink = xshort(1);
  din.size = xint(0);
  winode(inum, &din);
  return inum;
}

// Update the free block list by marking some blocks as in-use
void balloc(int used) {
  uchar buf[BSIZE];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < BSIZE * 8);
  memset(buf, 0, BSIZE);
  for (i = 0; i < used; i++) {
    buf[i / 8] = buf[i / 8] | (0x1 << (i % 8));
  }
  printf("balloc: write bitmap block at sector %d\n", nativesb.bmapstart);
  wsect(nativesb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

// Append more data to the file with i-node number inum
void iappend(uint inum, void *xp, int n) {
  char *p = (char *) xp;
  xvoff_t off;
  xvblk_t fbn, n1;
  struct dinode din;
  char buf[BSIZE];
  xvblk_t indirect[NINDIRECT];
  uint x;

  rinode(inum, &din);
  off = xint(din.size);
//printf("append inum %d at off %d sz %d\n", inum, off, n);
  while (n > 0) {
    fbn = off / BSIZE;
    assert(fbn < MAXFILE);
    if (fbn < NDIRECT) {
      if (xshort(din.addrs[fbn]) == 0) {
	din.addrs[fbn] = xshort(freeblock++);
      }
      x = xshort(din.addrs[fbn]);
    } else {
      if (xshort(din.addrs[NDIRECT]) == 0) {
// printf("Allocating block 0x%x as indirect block\n", freeblock);
	din.addrs[NDIRECT] = xshort(freeblock++);
      }
      rsect(xshort(din.addrs[NDIRECT]), (char *) indirect);
      if (indirect[fbn - NDIRECT] == 0) {
// printf("Allocating block 0x%x as indirect storage\n", freeblock);
	indirect[fbn - NDIRECT] = xshort(freeblock++);
	wsect(xshort(din.addrs[NDIRECT]), (char *) indirect);
      }
      x = xshort(indirect[fbn - NDIRECT]);
// printf("Allocating block 0x%x for file storage\n", x);
    }
    n1 = min(n, (fbn + 1) * BSIZE - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * BSIZE), n1);
// printf("Writing block 0x%x\n", x);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  assert(freeblock < FSSIZE);
  din.size = xint(off);
  winode(inum, &din);
}

// Add the given filename and i-number as a directory entry 
void dappend(int dirino, char *name, int fileino) {
  struct xvdirent de;

  memset(&de, 0, sizeof(de));
  de.inum = xshort(fileino);
  strncpy(de.name, name, DIRSIZ);
  iappend(dirino, &de, sizeof(de));
}

// Add a file to the directory with given i-num
void fappend(int dirino, char *filename, struct stat *sb) {
  char buf[BSIZE];
  int cc, fd, inum;

  // Open the file up
  if ((fd = open(filename, 0)) < 0) {
    perror(filename);
    exit(1);
  }
  // Allocate an i-node for the file
  inum = ialloc(T_FILE);

  // Add the file's name to the root directory
  dappend(dirino, filename, inum);

  // Read the file's contents in and write to the filesystem
  while ((cc = read(fd, buf, sizeof(buf))) > 0)
    iappend(inum, buf, cc);

  close(fd);
}

// Given a local directory name and a directory i-node number
// on the image, add all the files from the local directory
// to the on-image directory
void add_directory(int dirino, char *localdir) {
  DIR *D;
  struct dirent *dent;
  struct stat sb;
  int newdirino;

  D = opendir(localdir);
  if (D == NULL) {
    perror(localdir);
    exit(1);
  }
  chdir(localdir);

  while ((dent = readdir(D)) != NULL) {

    // Skip . and ..
    if (!strcmp(dent->d_name, "."))
      continue;
    if (!strcmp(dent->d_name, ".."))
      continue;

    if (stat(dent->d_name, &sb) == -1) {
      perror(dent->d_name);
      exit(1);
    }

    if (S_ISDIR(sb.st_mode)) {
      newdirino = makdir(dirino, dent->d_name, &sb);
      add_directory(newdirino, dent->d_name);
    }
    if (S_ISREG(sb.st_mode)) {
      fappend(dirino, dent->d_name, &sb);
    }
  }

  closedir(D);
  chdir("..");
}

// Make a directory entry in the directory with the given i-node number
// and return the new directory's i-number
int makdir(int dirino, char *newdir, struct stat *sb) {
  int ino;

  // Allocate the inode number for this directory
  // and set up the . and .. entries
  ino = ialloc(T_DIR);
  dappend(ino, ".", ino);
  dappend(ino, "..", dirino);

  // In the parent directory, add the new directory entry
  dappend(dirino, newdir, ino);

  return (ino);
}
