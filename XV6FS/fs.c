// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include <string.h>
#include <stdio.h>
#include <xv6/types.h>
#include <xv6/defs.h>
#include <xv6/param.h>
#include <xv6/stat.h>
#include <xv6/fs.h>
#include <xv6/buf.h>
#include <xv6/file.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
void itrunc(struct inode *);
struct superblock sb;

// The program's current directory
struct inode *cwd;		// Current directory

// Read the super block.
void readsb(struct superblock *sb) {
  struct buf *bp;

  bp = bread(1);
  rommemcpy(sizeof(*sb), bp->data, sb);
  brelse(bp);
}

// Zero a block.
static void bzero(xvblk_t bno) {
  struct buf *bp;

  bp = bread(bno);
  memset(bp->data, 0, BSIZE);
  // log_write(bp);
  bwrite(bp);
  brelse(bp);
}

// Blocks.

// Allocate a zeroed disk block.
static xvino_t balloc(void) {
  xvblk_t b, bi;
  Int m;
  struct buf *bp;

  bp = 0;
  for (b = 0; b < sb.size; b += BPB) {
    bp = bread(BBLOCK(b, sb));
    for (bi = 0; bi < BPB && b + bi < sb.size; bi++) {
      m = 1 << (bi % 8);
      if ((bp->data[(Int) (bi / 8)] & m) == 0) {	// Is block free?
	bp->data[(Int) (bi / 8)] |= (uchar) m;	// Mark block in use.
	// log_write(bp);
	bwrite(bp);
	brelse(bp);
	bzero(b + bi);
	return b + bi;
      }
    }
    brelse(bp);
  }
  panic("fs1");
  return (0);			// Keep -Wall happy
}

// Free a disk block.
static void bfree(xvblk_t b) {
  struct buf *bp;
  xvblk_t bi;
  Int m;

  bp = bread(BBLOCK(b, sb));
  bi = b % BPB;
  m = 1 << (bi % 8);
  if ((bp->data[(Int) (bi / 8)] & m) == 0)
    panic("fs2");
  bp->data[(Int) (bi / 8)] &= (uchar) ~ m;
  // log_write(bp);
  bwrite(bp);
  brelse(bp);
}

// Inodes.
//
// An inode describes a single unnamed file.
// The inode disk structure holds metadata: the file's type,
// its size, the number of links referring to it, and the
// list of blocks holding the file's content.
//
// The inodes are laid out sequentially on disk at
// sb.startinode. Each inode has a number, indicating its
// position on the disk.
//
// The kernel keeps a cache of in-use inodes in memory
// to provide a place for synchronizing access
// to inodes used by multiple processes. The cached
// inodes include book-keeping information that is
// not stored on disk: ip->ref and ip->valid.
//
// An inode and its in-memory representation go through a
// sequence of states before they can be used by the
// rest of the file system code.
//
// * Allocation: an inode is allocated if its type (on disk)
//   is non-zero. ialloc() allocates, and iput() frees if
//   the reference and link counts have fallen to zero.
//
// * Referencing in cache: an entry in the inode cache
//   is free if ip->ref is zero. Otherwise ip->ref tracks
//   the number of in-memory pointers to the entry (open
//   files and current directories). iget() finds or
//   creates a cache entry and increments its ref; iput()
//   decrements ref.
//
// * Valid: the information (type, size, &c) in an inode
//   cache entry is only correct when ip->valid is 1.
//   ilock() reads the inode from
//   the disk and sets ip->valid, while iput() clears
//   ip->valid if ip->ref has fallen to zero.
//
// * Locked: file system code may only examine and modify
//   the information in an inode and its content if it
//   has first locked the inode.
//
// Thus a typical sequence is:
//   ip = iget(inum)
//   ilock(ip)
//   ... examine and modify ip->xxx ...
//   iunlock(ip)
//   iput(ip)
//
// ilock() is separate from iget() so that system calls can
// get a long-term reference to an inode (as for an open file)
// and only lock it for short periods (e.g., in read()).
// The separation also helps avoid deadlock and races during
// pathname lookup. iget() increments ip->ref so that the inode
// stays cached and pointers to it remain valid.
//
// Many internal file system functions expect the caller to
// have locked the inodes involved; this lets callers create
// multi-step atomic operations.
//
// The icache.lock spin-lock protects the allocation of icache
// entries. Since ip->ref indicates whether an entry is free,
// and ip->inum indicate which i-node an entry
// holds, one must hold icache.lock while using any of those fields.
//
// An ip->lock sleep-lock protects all ip-> fields other than ref
// and inum.  One must hold ip->lock in order to
// read or write that inode's ip->valid, ip->size, ip->type, &c.

struct {
  struct inode inode[NINODE];
} icache;

void iinit(void) {
  readsb(&sb);
#if 0
  cprintf("sb: size %X nblocks %X ninodes %X nlog %X logstart %X\
 inodestart %X bmap start %X\n", sb.size, sb.nblocks, sb.ninodes, sb.nlog, sb.logstart, sb.inodestart, sb.bmapstart);
#endif

  // XXX: this belongs elsewhere
  cwd = namei("/");
}

static struct inode *iget(xvino_t inum);

//PAGEBREAK!
// Allocate an inode.
// Mark it as allocated by  giving it type type.
// Returns an unlocked but allocated and referenced inode.
struct inode *ialloc(short type) {
  xvino_t inum;
  struct buf *bp;
  struct dinode *dip;

  for (inum = 1; inum < sb.ninodes; inum++) {
    bp = bread(IBLOCK(inum, sb));
    dip = (struct dinode *) bp->data + inum % IPB;
    if (dip->type == 0) {	// a free inode
      memset(dip, 0, sizeof(*dip));
      dip->type = type;
      // log_write(bp);   // mark it allocated on the disk
      bwrite(bp);		// mark it allocated on the disk
      brelse(bp);
      return iget(inum);
    }
    brelse(bp);
  }
  panic("fs4");
  return ((struct inode *) 0);	// Keep -Wall happy
}

// Copy a modified in-memory inode to disk.
// Must be called after every change to an ip->xxx field
// that lives on disk, since i-node cache is write-through.
// Caller must hold ip->lock.
void iupdate(struct inode *ip) {
  struct buf *bp;
  struct dinode *dip;

  bp = bread(IBLOCK(ip->inum, sb));
  dip = (struct dinode *) bp->data + ip->inum % IPB;
  dip->type = ip->type;
  dip->nlink = ip->nlink;
  dip->size = ip->size;
  rommemcpy(sizeof(ip->addrs), ip->addrs, dip->addrs);
  // log_write(bp);
  bwrite(bp);
  brelse(bp);
}

// Find the inode with number inum
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
static struct inode *iget(xvino_t inum) {
  struct inode *ip, *empty;

  // Is the inode already cached?
  empty = 0;
  for (ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++) {
    if (ip->ref > 0 && ip->inum == inum) {
      ip->ref++;
      return ip;
    }
    if (empty == 0 && ip->ref == 0)	// Remember empty slot.
      empty = ip;
  }

  // Recycle an inode cache entry.
  if (empty == 0)
    panic("fs5");

  ip = empty;
  ip->inum = inum;
  ip->ref = 1;
  ip->valid = 0;

  return ip;
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode *idup(struct inode *ip) {
  ip->ref++;
  return ip;
}

// Lock the given inode.
// Reads the inode from disk if necessary.
void ilock(struct inode *ip) {
  struct buf *bp;
  struct dinode *dip;

  if (ip == 0 || ip->ref < 1)
    panic("fs6");

  if (ip->valid == 0) {
    bp = bread(IBLOCK(ip->inum, sb));
    dip = (struct dinode *) bp->data + ip->inum % IPB;

    ip->type = dip->type;
    ip->nlink = dip->nlink;
    ip->size = dip->size;
    rommemcpy(sizeof(ip->addrs), dip->addrs, ip->addrs);
    brelse(bp);
    ip->valid = 1;
    if (ip->type == 0)
      panic("fs7");
  }
}

// Unlock the given inode.
void iunlock(struct inode *ip) {
}

// Drop a reference to an in-memory inode.
// If that was the last reference, the inode cache entry can
// be recycled.
// If that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk.
// All calls to iput() must be inside a transaction in
// case it has to free the inode.
void iput(struct inode *ip) {
  if (ip->valid && ip->nlink == 0) {
    Int r = ip->ref;
    if (r == 1) {
      // inode has no links and no other references: truncate and free.
      itrunc(ip);
      ip->type = 0;
      iupdate(ip);
      ip->valid = 0;
    }
  }

  ip->ref--;
}

// Common idiom: unlock, then put.
void iunlockput(struct inode *ip) {
  iunlock(ip);
  iput(ip);
}

//PAGEBREAK!
// Inode content
//
// The content (data) associated with each inode is stored
// in blocks on the disk. The first NDIRECT block numbers
// are listed in ip->addrs[].  The next NINDIRECT blocks are
// listed in block ip->addrs[NDIRECT].

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
static xvblk_t bmap(struct inode *ip, xvblk_t bn) {
  xvblk_t addr, *a;
  struct buf *bp;

  if (bn < NDIRECT) {
    if ((addr = ip->addrs[(Int) bn]) == 0)
      ip->addrs[(Int) bn] = addr = balloc();
    return addr;
  }
  bn -= NDIRECT;

  if (bn < NINDIRECT) {
    // Load indirect block, allocating if necessary.
    if ((addr = ip->addrs[NDIRECT]) == 0)
      ip->addrs[NDIRECT] = addr = balloc();
    bp = bread(addr);
    a = (xvblk_t *) bp->data;
    if ((addr = a[(Int) bn]) == 0) {
      a[(Int) bn] = addr = balloc();
      // log_write(bp);
      bwrite(bp);
    }
    brelse(bp);
    return addr;
  }

  panic("fs8");
  return (0);			// Keep -Wall happy
}

// Truncate inode (discard contents).
// Only called when the inode has no links
// to it (no directory entries referring to it)
// and has no in-memory reference to it (is
// not an open file or current directory).
void itrunc(struct inode *ip) {
  Int i, j;
  struct buf *bp;
  xvblk_t *a;

  for (i = 0; i < NDIRECT; i++) {
    if (ip->addrs[i]) {
      bfree(ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  if (ip->addrs[NDIRECT]) {
    bp = bread(ip->addrs[NDIRECT]);
    a = (xvblk_t *) bp->data;
    for (j = 0; j < NINDIRECT; j++) {
      if (a[j])
	bfree(a[j]);
    }
    brelse(bp);
    bfree(ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }

  ip->size = 0;
  iupdate(ip);
}

// Copy stat information from inode.
// Caller must hold ip->lock.
void stati(struct inode *ip, struct xvstat *st) {
  st->ino = ip->inum;
  st->type = ip->type;
  st->nlink = ip->nlink;
  st->size = ip->size;
}

//PAGEBREAK!
// Read data from inode.
// Caller must hold ip->lock.
xvoff_t readi(struct inode *ip, char *dst, xvoff_t off, xvoff_t n) {
  Uint tot, m;
  struct buf *bp;

  if (off > ip->size || off + n < off)
    return -1;
  if (off + n > ip->size)
    n = ip->size - off;

  for (tot = 0; tot < n; tot += m, off += m, dst += m) {
    // bp = bread(bmap(ip, off / BSIZE));
    // m = min((Uint) (n - tot), (Uint) (BSIZE - off % BSIZE));
    // memmove(dst, bp->data + off % BSIZE, m);
    bp = bread(bmap(ip, off >> 9));
    m = min((Uint) (n - tot), (Uint) (BSIZE - (off & (BSIZE-1))));
    rommemcpy(m, bp->data + (off & (BSIZE-1)), dst);
    brelse(bp);
  }
  return n;
}

// PAGEBREAK!
// Write data to inode.
// Caller must hold ip->lock.
xvoff_t writei(struct inode *ip, char *src, xvoff_t off, xvoff_t n) {
  xvoff_t tot, m;
  struct buf *bp;

  if (off > ip->size || off + n < off)
    return -1;
  if (off + n > (xvoff_t)MAXFILE * (xvoff_t)BSIZE)
    return -1;

  for (tot = 0; tot < n; tot += m, off += m, src += m) {
    // bp = bread(bmap(ip, off / BSIZE));
    // m = min((Uint) (n - tot), (uint) (BSIZE - off % BSIZE));
    // memmove(bp->data + off % BSIZE, src, m);
    bp = bread(bmap(ip, off >> 9));
    m = min((n - tot), (BSIZE - (off & (BSIZE-1))));
    rommemcpy(m, src, bp->data + (off & (BSIZE-1)));
    // log_write(bp);
    bwrite(bp);
    brelse(bp);
  }

  if (n > 0 && off > ip->size) {
    ip->size = off;
    iupdate(ip);
  }
  return n;
}

//PAGEBREAK!
// Directories

Int namecmp(const char *s, const char *t) {
  return strncmp(s, t, DIRSIZ);
}

// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode *dirlookup(struct inode *dp, char *name, xvoff_t * poff) {
  xvoff_t off;
  xvino_t inum;
  struct xvdirent de;

  if (dp->type != T_DIR)
    panic("fs9");

  for (off = 0; off < dp->size; off += sizeof(de)) {
    if (readi(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
      panic("fs10");
    if (de.inum == 0)
      continue;
    if (namecmp(name, de.name) == 0) {
      // entry matches path element
      if (poff)
	*poff = off;
      inum = de.inum;
      return iget(inum);
    }
  }

  return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
Int dirlink(struct inode *dp, char *name, xvino_t inum) {
  Int off;
  struct xvdirent de;
  struct inode *ip;

  // Check that name is not present.
  if ((ip = dirlookup(dp, name, 0)) != 0) {
    iput(ip);
    return -1;
  }
  // Look for an empty xvdirent.
  for (off = 0; off < dp->size; off += sizeof(de)) {
    if (readi(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
      panic("fs11");
    if (de.inum == 0)
      break;
  }

  strncpy(de.name, name, DIRSIZ);
  de.inum = inum;
  if (writei(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
    panic("fs12");

  return 0;
}

//PAGEBREAK!
// Paths

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static char *skipelem(char *path, char *name) {
  char *s;
  Int len;

  while (*path == '/')
    path++;
  if (*path == 0)
    return 0;
  s = path;
  while (*path != '/' && *path != 0)
    path++;
  len = path - s;
  if (len >= DIRSIZ)
    rommemcpy(DIRSIZ, s, name);
  else {
    rommemcpy(len, s, name);
    name[len] = 0;
  }
  while (*path == '/')
    path++;
  return path;
}

// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Must be called inside a transaction since it calls iput().
static struct inode *namex(char *path, xvino_t nameiparent, char *name) {
  struct inode *ip, *next;

  if (*path == '/')
    ip = iget(ROOTINO);
  else
    ip = idup(cwd);

  while ((path = skipelem(path, name)) != 0) {
    ilock(ip);
    if (ip->type != T_DIR) {
      iunlockput(ip);
      return 0;
    }
    if (nameiparent && *path == '\0') {
      // Stop one level early.
      iunlock(ip);
      return ip;
    }
    if ((next = dirlookup(ip, name, 0)) == 0) {
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
  }
  if (nameiparent) {
    iput(ip);
    return 0;
  }
  return ip;
}

struct inode *namei(char *path) {
  char name[DIRSIZ];
  return namex(path, 0, name);
}

struct inode *nameiparent(char *path, char *name) {
  return namex(path, 1, name);
}
