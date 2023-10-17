// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
//
// The implementation uses two state flags internally:
// * B_VALID: the buffer data has been read from the disk.
// * B_DIRTY: the buffer data has been modified
//     and needs to be written to disk.

#include <sys/types.h>
#include <xv6/types.h>
#include <xv6/defs.h>
#include <xv6/param.h>
#include <xv6/fs.h>
#include <xv6/buf.h>

struct {
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head;
} bcache;

void binit(void) {
  struct buf *b;

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// Look through buffer cache for a block.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *bget(xvblk_t blockno) {
  struct buf *b;

  // Is the block already cached?
  for (b = bcache.head.next; b != &bcache.head; b = b->next) {
    if (b->blockno == blockno) {
      b->refcnt++;
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  // Even if refcnt==0, B_DIRTY indicates a buffer is in use
  // because log.c has modified it but not yet committed it.
  for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
    if (b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
      b->blockno = blockno;
      b->flags = 0;
      b->refcnt = 1;
      return b;
    }
  }
  panic("bi1");
  return ((struct buf *) 0);	// Keep -Wall happy
}

// Return a locked buf with the contents of the indicated block.
struct buf *bread(xvblk_t blockno) {
  struct buf *b;

  b = bget(blockno);
  if ((b->flags & B_VALID) == 0) {
    blkrw(b);
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b) {
  b->flags |= B_DIRTY;
  blkrw(b);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void brelse(struct buf *b) {
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

//PAGEBREAK!
// Blank page.
