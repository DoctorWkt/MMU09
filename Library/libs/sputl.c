#include <stdint.h>
#include <stdio.h>

void sputl(long value, char *buffer)
{
    uint8_t *p = (uint8_t *)buffer;
    uint32_t r = (uint32_t)value;
    
    *p++ = (uint8_t)r;
    r <<= 8;
    *p++ = (uint8_t)r;
    r <<= 8;
    *p++ = (uint8_t)r;
    r <<= 8;
    *p = (uint8_t)r;
}
