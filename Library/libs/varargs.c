/*  varargs.c - Support for variable argument functions

    By Pierre Sarrazin <http://sarrazip.com/>.
    This file is in the public domain.
*/

#include "stdarg.h"


char *__va_arg(va_list *app, unsigned int sizeInBytes)
{
    char *origAddr = (char *) *app;
    if (sizeInBytes == 1)  // if char type or 1-byte struct
        ++origAddr;  // skip MSB of argument in stack; point to LSB
    *app = (va_list) (origAddr + sizeInBytes);
    return origAddr;
}
