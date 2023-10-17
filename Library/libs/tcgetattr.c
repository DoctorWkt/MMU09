#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <romcalls.h>


int tcgetattr(int fd, struct termios *termios_p)
{
  int termattr;

  // Error check
  errno= 0;
  if (!isatty(fd)) { errno= ENOTTY; return(-1); }
  if (termios_p==NULL) { errno= EINVAL; return(-1); }

  // Get the terminal attributes
  termattr= _tcattr(0, 0);

  // At the moment, all we have is ECHO
  termios_p->c_iflag= 0;
  termios_p->c_oflag= 0;
  termios_p->c_cflag= 0;
  termios_p->c_lflag= termattr;
  memset(termios_p->c_cc, 0, NCCS);
  return(0);
}
