#ifndef __SETJMP_H
#define __SETJMP_H
#ifndef __TYPES_H
#include <types.h>
#endif

typedef uint16_t jmp_buf[4];
extern int setjmp(jmp_buf __env);
extern void longjmp(jmp_buf __env, int __rv);

#endif
