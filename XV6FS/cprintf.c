#include <string.h>
#include <errno.h>
#define CPRINTF_REDEFINED
#include <xv6/types.h>
#include <xv6/defs.h>
#include <xv6/param.h>
#include <xv6/stat.h>
#include <xv6/fs.h>
#include <xv6/file.h>
#include <xv6/fcntl.h>

static const char digits[] = "0123456789abcdef";

static void printint(int xx, int base, int sign) {
  char buf[16];
  int i;
  uint x;

  if (sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[(unsigned int) (x % base)];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    romputc(buf[i]);
}

// Print to the console. only understands %d, %x, %p, %s.
// The dummy argument is to get the argp to work
void cprintf(char *fmt, uint dummy, ...) {
  int i, c, locking;
  uint *argp;
  char *s;

  if (fmt == 0)
    // panic("null fmt");
    return;

  // argp = (uint *) (void *) (&fmt + 1);
  argp = (uint *) &dummy;
  for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
    if (c != '%') {
      romputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if (c == 0)
      break;
    switch (c) {
    case 'c':
      romputc((char) (*argp & 0xff)); argp++;
      break;
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'o':
      printint(*argp++, 8, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if ((s = (char *) *argp++) == 0)
	s = "(null)";
      for(; *s; s++)
        romputc(*s);
      break;
    case '%':
      romputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      romputc('%');
      romputc(c);
      break;
    }
  }
}
