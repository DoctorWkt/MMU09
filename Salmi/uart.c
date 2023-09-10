// Code to deal with the UART
// (c) 2023 Warren Toomey, GPL3

#include "6809.h"
#include <sys/types.h>
#include <fcntl.h>

// Original blocking terminal setting
struct termios orig_termios;

// We use a character buffer to get as much input from
// the keyboard (so we don't lose any), and then we can
// replay it to the simulated code
#define CBUFSIZE 4096
unsigned char termbuf[CBUFSIZE];
int charsleft=0;
int termbufindex=0;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void save_old_terminal_mode()
{
    tcgetattr(0, &orig_termios);
}

int ttySetCbreak(void)
{
    struct termios t;

    if (tcgetattr(0, &t) == -1)
        return -1;

    t.c_lflag &= ~(ICANON | ECHO);
    t.c_lflag |= ISIG;

    t.c_iflag &= ~ICRNL;

    t.c_cc[VMIN] = 0;                   /* Character-at-a-time input */
    t.c_cc[VTIME] = 0;                  /* with blocking */

    if (tcsetattr(0, TCSAFLUSH, &t) == -1)
        return -1;

    return 0;
}

// Return 1 if there is a character ready to read from the
// keyboard, otherwise return 0.
int kbhit()
{
    int r;

    // We have characters buffered
    if (charsleft >0) return(1);

    // OK, try to get some characters into the buffer
    // Return empty if there was nothing to read
    if ((r=read(0, &termbuf, CBUFSIZE)) <1)
      return(0);

    // We read one or more characters, so update the charsleft
    // and return that we have characters to read
    charsleft= r;
    termbufindex=0;
    return(1);
}

// Read a character from the keyboard. This should be
// done in the IRQ handler so that we know something
// will be read. Returns 0 if no characters available.
unsigned kbread(void) {
  // We have no characters; try to get some
  if (charsleft == 0) kbhit();
  // Still no characters, return something useless
  if (charsleft == 0) return(0);

// printf("%d charsleft, index %d, char %c\n", charsleft, termbufindex,
// 	termbuf[ termbufindex]);

  // At this point we have charsleft > 0
  // Decrement the number of characters and return one of them
  charsleft--;
  return(termbuf[ termbufindex++ ]);
}
