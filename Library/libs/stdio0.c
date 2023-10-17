#include "stdio-l.h"

/* TODO sdcc err #define buferr (unsigned char *)(stderr->unbuf)*//* Stderr is unbuffered */

FILE *__IO_list = NULL;		/* For fflush at exit */

static unsigned char bufin[BUFSIZ];
static unsigned char bufout[BUFSIZ];
#ifndef buferr
static unsigned char buferr[BUFSIZ];
#endif
FILE stdinstruct = {
	bufin, bufin, bufin, bufin, bufin + BUFSIZ,
	 0, _IOFBF | __MODE_READ | __MODE_IOTRAN,
	"       ", (FILE *)0
};

FILE stdoutstruct = {
	bufout, bufout, bufout, bufout, bufout + BUFSIZ,
	 1, _IOFBF | __MODE_WRITE | __MODE_IOTRAN,
	"       ", (FILE *)0
};

FILE stderrstruct = {
	buferr, buferr, buferr, buferr, buferr + BUFSIZ,
	 2, _IONBF | __MODE_WRITE | __MODE_IOTRAN,
	"       ", (FILE *)0
};

FILE *stdin  = &stdinstruct;
FILE *stdout = &stdoutstruct;
FILE *stderr = &stderrstruct; 

/* Call the stdio initialiser; it's main job it to call atexit */

STATIC void __stdio_close_all(void)
{
	FILE *fp = __IO_list;

	fflush(stdout);
	fflush(stderr);
	while (fp) {
		fflush(fp);
		close(fp->fd);
		/* Note we're not de-allocating the memory */
		/* There doesn't seem to be much point :-) */
		fp->fd = -1;
		fp = fp->next;
	}
}

STATIC void __stdio_init_vars(void)
{
	if (isatty(1))
		stdout->mode |= _IOLBF;
	atexit((atexit_t) __stdio_close_all);
}
