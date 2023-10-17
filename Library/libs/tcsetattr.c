#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <romcalls.h>


int tcsetattr(int fd, int oa, const struct termios *termios_p)
{
  int termattr;

  // Error check
  errno= 0;
  if (!isatty(fd)) { errno= ENOTTY; return(-1); }
  if (termios_p==NULL) { errno= EINVAL; return(-1); }

  // Isolate only the echo flag
  termattr= termios_p->c_lflag & TC_ECHO;

  // Set the terminal attributes
  _tcattr(1, termattr);
  return(0);
}

