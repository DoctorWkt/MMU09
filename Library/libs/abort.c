/*
 *	FIXME: should do an fflushall IFF stdio is in use
 */
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <syscalls.h>

void abort(void)
{
	_exit(255);
}
