/* This little wrapper seems needed when linking with libgcc */

#include <unistd.h>

int putchar(int ch)
{
        unsigned char c = (unsigned char)ch;
        return write(1, &c, 1);
}
