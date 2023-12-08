#define CPRINTF_REDEFINED
#include <string.h>
#include <stdio.h>
#include <romcalls.h>

static void printint(int xx, int base, int sign) {
  static char digits[] = "0123456789abcdef";
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
void cprintf(char *fmt, uint fred, ...) {
  int i, c, locking;
  uint *argp;
  char *s;

  if (fmt == 0)
    // panic("null fmt");
    return;

  // argp = (uint *) (void *) (&fmt + 1);
  argp = (uint *) &fred;
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
