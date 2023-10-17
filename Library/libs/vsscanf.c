/*
 * This file based on scanf.c from 'Dlibs' on the atari ST  (RdeBath)
 *
 * 19-OCT-88: Dale Schumacher
 * > John Stanley has again been a great help in debugging, particularly
 * > with the printf/scanf functions which are his creation.  
 *
 *    Dale Schumacher                         399 Beacon Ave.
 *    (alias: Dalnefre')                      St. Paul, MN  55104
 *    dal@syntel.UUCP                         United States of America
 *  "It's not reality that's important, but how you perceive things."
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

static FILE strstruct = {
        (uchar *)0,             // cmoc complains if we use NULL here
        (uchar *)0,
        (uchar *) -1,
        (uchar *)0,
        (uchar *) -1,
        -1,
        _IOFBF | __MODE_WRITE,
        "       ",
        (FILE *)0
};

static FILE *string= &strstruct;

int vsscanf(const char * sp, const char *fmt, va_list ap)
{
  string->bufpos = (unsigned char *)sp;
  return vfscanf(string,fmt,ap);
}
