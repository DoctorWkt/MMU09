// Access the CH375 block device

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <xv6/types.h>
#include <xv6/defs.h>
#include <xv6/param.h>
#include <xv6/fs.h>
#include <xv6/buf.h>

// Read/write a buffer
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void blkrw(struct buf *b) {
  if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
    panic("bl2");

  if (b->flags & B_DIRTY) {
    if (writeblock(b->data, b->blockno) == 0)
      panic("bl3");
    b->flags &= ~B_DIRTY;
  } else {
    if (readblock(b->data, b->blockno) == 0)
      panic("bl4");
  }
  b->flags |= B_VALID;
}
