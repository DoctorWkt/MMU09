#include <string.h>

void *memcpy(void *dest, const void *src, size_t len)
{
	uint8_t *dp = (uint8_t *)dest;
	const uint8_t *sp = (const uint8_t *)src;
	while(len-- > 0)
		*dp++=*sp++;
	return dest;
}
