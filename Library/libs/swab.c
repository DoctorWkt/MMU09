#include <unistd.h>

void swab(const void *from, void *to, ssize_t n)
{
  const uint8_t *f = (const uint8_t *)from;
  uint8_t *t = (uint8_t *)to;
  n >>= 1;
  while(n--) {
    *t++ = f[1];
    *t++ = *f;
    f += 2;
  }
}
