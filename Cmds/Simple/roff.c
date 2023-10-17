/* Warren's version of Doug McIlroy's RATFOR roff.
 * (c) 2016 Warren Toomey, GPL3.
 */
#define EOFILE EOF
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define Int		short
void printn(Int x)
{
    printf("%06o\n", x);
}

#define Char		short
#define HUGE		1000
#define INSIZE		300	// size of input buffer
#define MAXLINE		100
#define MAXOUT		300
#define PAGELEN		66
#define PAGEWIDTH	60
#define ULMASK		0400	// Underline bit mask
#define BOLDMASK	0200	// Bold bit mask
#define ASCIIMASK	0177	// Mask for ASCII characters
#define EXTRASPACE	01000	// Value to indicate an extra space
#define MAXTABS		20	// Maximum tabs in the table

Char inbuf[INSIZE];		// input line buffer
Char outbuf[MAXOUT];		//  lines to be filled collect here
Char header[MAXLINE];		//  top of page title
Char footer[MAXLINE];		//  bottom of page title
Char tempbuf[10];		// use by itoa()
Int tablist[] = {
    8, 16, 24, 32, 40, 48, 56, 64, 72, 80,
    88, 96, 104, 112, 120, 128, 136, 144, 152, 160
};

Int direction = 1;		// direction to spread spaces, 1 or -1
Int bottom;			// last live line on page, = plval-m3val-m4val
Int boval=0;			// number of lines to embolden
Int ceval = 0;			// number of lines to center
Int curpag = 0;			// current output page number
Int fill = 1;			// fill the line? 1 is yes
Int inval = 0;			// current indent
Int justify = 1;		// justify lines? 1 is yes
Int lineno = 0;			// next line to be printed
Int lival = 0;			// number of lines to treat literally
Int lsval = 1;			// current line spacing
Int m1val = 3;			// margin before and including header
Int m2val = 2;			// margin after header
Int m3val = 2;			// margin after last text line
Int m4val = 3;			// bottom margin, including footer
Int ntabs = 20;			// number of tabs in the list
Int newpag = 1;			// next output page number
Char *outp;			// last Char position in outbuf
Int outlen;			// current length of output line
Int plval = PAGELEN;		// page length in lines
Int rmval = PAGEWIDTH;		// current right margin
Int tival = 0;			// current temporary indent
Int ulval = 0;			// number of lines to underline
Char hypchar='\0';		// character to identy hyphenation points
Int hyplen= 0;			// word length up to & including hypchar


void command();
void text(Char * inbuf, Char * outbuf);
void put(Char * buf);
void spread(Char * buf, Int nextra);
Int getval(Char * buf, Int defval, Int minval, Int maxval);
void do_ta(Char * buf);
void gettl(Char * buf, Char * ttl);
Char getlin(Char * buf, Int maxlen, Int fd);
void brkline();
void space(Int n);
void pfoot();
void phead();
void puttl(Char * t, Int pageno);
Char *partitle(Char * t, Char sep, Char * pgnum);
void skip(Int n);
Int ctoi(Char * in);
Int Strlen(Char * c);
Int Wordlen(Char * c);
Int is_underl(Char ch);
void Strcpy(Char * dst, Char * src);
void Wordcopy(Char * dst, Char * src, Int n);
Char *itoa(Int n);
Int titlen(Char * t, Char c, Int k);
Int min(Int a, Int b);
void Puts(Char * c);

int main(int argc, char *argv[])
{
    int fd;
    // Initialise some values
    bottom = plval - m3val - m4val;
    header[0] = '\0';
    footer[0] = '\0';
    outp = outbuf;
    outlen = 0;
    fd = 0;

    if (argc==2) {
      fd= open(argv[1], O_RDONLY);
      if (fd == -1) {
        fprintf(stderr, "Unable to open %s\n", argv[1]); exit(1);
      }
    }

    // For each input line
    while (getlin(inbuf, INSIZE, fd) != EOFILE) {

	// Deal with a command when it's not a literal line
	if (*inbuf == '.' && lival==0)
	    command();
	else {
	    text(inbuf, outbuf); // or add it to the outbuf buffer
	    if (lival) lival--;	// That's another literal line done
	}
    }
    if (lineno > 0)		// Flush the last page if needed
	space(HUGE);
    return(0);
}

// Process a command on an input line
void command()
{
    Int cmd = inbuf[1] << 8 | inbuf[2];
    Int spval;
    Char *cptr = inbuf + 3;		// points past the command

    if (cmd == 0x6669) {		// 'fi', set fill mode
	brkline();
	fill = 1;
    } else if (cmd == 0x6e66) {		// 'nf', disable fill mode
	brkline();
	fill = 0;
    } else if (cmd == 0x6a75) {		// 'ju', set justify mode
	justify = 1;
    } else if (cmd == 0x6e6a) {		// 'nj', disable justify mode
	justify = 0;
    } else if (cmd == 0x6272)		// 'br', break current line
	brkline();
    else if (cmd == 0x6c73)		// 'ls'
	lsval = getval(cptr, 1, 1, HUGE);
    else if (cmd == 0x6270) {		// 'bp'
	if (lineno > 0)
	    space(HUGE);
	curpag = getval(cptr, curpag + 1, -HUGE, HUGE);
	newpag = curpag;
    } else if (cmd == 0x7370) {		// 'sp'
	spval = getval(cptr, 1, 0, HUGE);
	space(spval);
    } else if (cmd == 0x696e) {		// 'in', set the indent value
	inval = getval(cptr, 0, 0, rmval);
	tival = inval;
    } else if (cmd == 0x726d)		// 'rm', set the right margin
	rmval = getval(cptr, PAGEWIDTH, tival + 1, HUGE);
    else if (cmd == 0x7469) {		// 'ti', set a temporary indent
	brkline();
	tival = getval(cptr, 0, 0, rmval);
    } else if (cmd == 0x6365) {		// 'ce', centre lines
	brkline();
	ceval = getval(cptr, 1, 0, HUGE);
    } else if (cmd == 0x756c) {		// 'ul', underline lines
	ulval = getval(cptr, 1, 1, HUGE);
    } else if (cmd == 0x626f) {		// 'bo', embolden lines
	boval = getval(cptr, 1, 1, HUGE);
    } else if (cmd == 0x6865) {		// 'he'
	gettl(cptr, header);
    } else if (cmd == 0x666f) {		// 'fo'
	gettl(cptr, footer);
    } else if (cmd == 0x706c) {		// 'pl', set page length value
	plval = getval(cptr, PAGELEN,
		       m1val + m2val + m3val + m4val + 1, HUGE);
	bottom = plval - m3val - m4val;
    } else if (cmd == 0x7461)		// 'ta', set tab values
	do_ta(cptr);
      else if (cmd == 0x6c69)		// 'li', set literal count
	lival = getval(cptr, 1, 0, HUGE);
      else if (cmd == 0x6863) {		// 'hc', set hyphenation character
	// Skip whitespace
	while (*cptr == ' ' || *cptr == '\t') cptr++;
	hypchar= *cptr;
      }
}

// Deal with a new input line of text. We may prInt multiple
// output lines in the process, or we may not prInt anything
void text(Char * inbuf, Char * outbuf)
{
    Char *inp = inbuf;		// Start at the beginning of the input buffer
    Int linelen = rmval - tival; // Working length of the line
    Int i, len, nextra;
    Int tabposn, tabentry = 0;
    Int spacecount;
    Char *x;

    // Underline non-space characters, embolden all characters as required
    while (*inp) {
        if (ulval) {
	    if (is_underl(*inp))
		// Mark the underlined characters with the ULMASK
		*inp = *inp | ULMASK;
	}
        if (boval) {
		*inp = *inp | BOLDMASK;
  	}
	inp++;
    }

    if (ulval) ulval--;
    if (boval) boval--;
    inp = inbuf;		// Reset for next piece of code

    while (*inp) {		// While the input buffer has something
	// Skip spaces
	if (*inp == ' ') {
	    inp++;
	    continue;
	}

	// Expand tabs
	if (*inp == '\t') {
	    // Get the next tab position
	    tabposn = tablist[tabentry++];
	    if (tabentry == MAXTABS)
		tabentry--;

	    // Work out how many extra spaces to add, and add them
	    // We include tival so that it's absolute from column 1
	    spacecount = tabposn - outlen - tival;
	    for (i = 1; i < spacecount; i++) {
		*outp = ' ';
		outp++;
		outlen++;
	    }
	    inp++;
	    continue;
	}

	// We are at the beginning of a word. Get its length
	len = Wordlen(inp);

	// Work out how many extra characters are needed on the output line
	// Subtract 1 if we already have something, to account for a space char
	nextra = linelen - outlen;
	if (outlen) nextra--;

	// Special case: the word is larger than the linelen, so
	// it will never fit on any line. Set len so that we
	// forcibly break the word into two sections
	if (len > linelen)
	    len = nextra;

	// If the word is too long but we have a hyphenation point
	// that will fit, use the hypehnation length. Change the
	// hyphenation character to a '-'.
	if (len > nextra && hyplen != 0 && hyplen <= nextra) {
	    len= hyplen;
	    inp[ hyplen - 1  ]= '-';
	    hyplen=0;	// It's no longer a hypchar to skip below
	}

	// If we can squeeze a space and the word onto the line, do it
	if (nextra >= len) {

	    // Add a space if this isn't the beginning of the line
	    if (outlen) {
		*outp = ' ';
		outp++; outlen++;
	    }

	    // Copy the word and bump up the pointers
	    // Also account for any hyphenation character
	    Wordcopy(outp, inp, len);
	    inp = inp + len;
	    if (hyplen) inp++;
	    outp = outp + len;
	    outlen = outlen + len;
	} else {

	    // We can't add any more to the line
	    // Spread the line if justify is on and centre is off
	    if (justify && ceval == 0) {
		nextra = linelen - outlen;
		spread(outbuf, nextra);
	    }

	    // And prInt it out
	    brkline();
	}
    }

    // We have reached the end of the input line. Break if no fill
    if (!fill)
	brkline();
}

// put: put out line with proper spacing and indenting
void put(Char * buf)
{
    Int i, nextra;
    Char ch, ulflag, boflag;

    // PrInt out the header
    if (lineno == 0 || lineno > bottom)
	phead();

    // Centering is done by setting a temporary indent value
    if (ceval) {
	i= Strlen(buf);		// XXX Why can't I put this below?
				// Because the f'n call tromps on temps
	tival = (rmval + tival - i) / 2;
	if (tival < 0)
	    tival = 0;
	ceval--;
    }

    // PrInt out the temporary indent spaces
    // and reset back to the usual indent value
    for (i = 0; i < tival; i++)
	putchar(' ');
    tival = inval;

    // Now process each character one at a time
    while (1) {

	// Get the actual ASCII value and the underline flag
	ch = *buf & ASCIIMASK;
	ulflag = *buf & ULMASK;
	boflag = *buf & BOLDMASK;
	if (ch == '\0')
	    break;

	// If this is a space, prInt out any extra spaces
	// by getting the extra space count in the upper half
	// of the word
	if (ch == ' ') {
	    nextra = *buf >> 9;
	    for (i = 0; i != nextra; i++)
		putchar(' ');
	}
	// Underline the character if the flag is set
	// Do the same for bold characters
	if (ulflag) {
	    putchar('_');
	    putchar(8);		// 8 is ASCII backspace
	}
	if (boflag) {
	    putchar(ch);
	    putchar(8);		// 8 is ASCII backspace
	}
	putchar(ch);
	buf++;
    }

    // End with a newline
    putchar('\n');

    // Do any end of page padding
    i = min(lsval - 1, bottom - lineno);
    skip(i);

    // PrInt out the footer if required
    lineno = lineno + lsval;
    if (lineno > bottom)
	pfoot();
}


// Mark up the spaces in the buffer so that there are nextra spaces
// Do this by storing an Int value in the upper half of the word
void spread(Char * buf, Int nextra)
{
    Int buflen = Strlen(buf);
    Char *start;
    Char *end;
    Char *cptr;
    Char ch;

    // Set up the start and end based on the current direction
    if (direction == 1) {
	start = buf;
	end = buf + buflen;
    } else {
	end = buf;
	start = buf + buflen;
    }

    // Loop until we have added nextra spaces
    while (nextra > 0) {
	for (cptr = start; cptr != end; cptr = cptr + direction) {
	    // Get the ASCII character
	    ch = *cptr & ASCIIMASK;

	    // If the actual character is a space
	    if (nextra && ch == ' ') {
		*cptr = *cptr + EXTRASPACE;	// Add an extra space marker
		nextra--;
	    }
	}
    }
    direction = 0 - direction;			// Swap direction for next time
}

// Get an integer value following a command
// Check the range, and possibly set a default value
Int getval(Char * buf, Int defval, Int minval, Int maxval)
{
    Int val;

    // Skip any spaces
    while (*buf == ' ' || *buf == '\t')
	buf++;

    // Return the default
    if (*buf == '\0')
	return (defval);

    // Get the value and ensure it is within range
    val = ctoi(buf);
    if (val < minval)
	val = minval;
    if (val > maxval)
	val = maxval;
    return (val);
}

// Get the tab positions from the input line and store them in
// the tablist
void do_ta(Char * buf)
{
    Int i;
    Int val;

    for (i = 0; i < MAXTABS; i++) {
	// Skip leading spaces
	while (*buf == ' ' || *buf == '\t' || *buf == '\0') {
	    if (*buf == '\0')
		return;
	    buf++;
	}
	val = ctoi(buf);
	tablist[i] = val;
	// Now skip the number
	while (*buf != ' ' && *buf != '\t' && *buf != '\0') {
	    buf++;
	}
	if (*buf == '\0')
	    return;
    }
}

//  gettl: copy title from buf to ttl
void gettl(Char * buf, Char * ttl)
{
    Int i;

    // Skip any spaces and tabs
    while (*buf == ' ' || *buf == '\t')
	buf++;
    Strcpy(ttl, buf);
    return;
}


// Read a line of text from the given fd into inbuf.
// Return EOFILE on end of line, otherwise not EOFILE.
// Ensure the line is NUL terminated.
Char getlin(Char * buf, Int maxlen, Int fd)
{
    Int i, err;
    Char c;
    char c2;
    for (i = 0; i < maxlen - 1; i++) {
	err= read(fd, &c2, 1); c= c2;
	if (err != 1) c = EOFILE;
	if (c == EOFILE || c == '\n')
	    break;
	*buf = c;
	buf++;
    }
    *buf = '\0';
    return (c);
}

//  brkline: end and prInt current filled line
void brkline()
{
    // PrInt the line if something is in it
    if (outlen)
	put(outbuf);

    // Now an empty line
    outp = outbuf;
    outlen = 0;
    return;
}

// space: space n lines or to bottom of page
void space(Int n)
{
    Int s;

    // PrInt out any remaining output line
    brkline();

    // Stop now, or prInt the header as required
    if (lineno > bottom)
	return;
    if (lineno == 0)
	phead();

    // Work out how many empty lines to print
    s = min(n, bottom + 1 - lineno);
    skip(s);

    // And prInt the footer as required
    lineno = lineno + n;
    if (lineno > bottom)
	pfoot();
    return;
}

// pfoot: put out page footer
void pfoot()
{
    skip(m3val);
    if (m4val > 0) {
	puttl(footer, curpag);
	skip(m4val - 1);
    }
}

// phead: put out page header
void phead()
{
    curpag = newpag;
    newpag++;
    if (m1val > 0) {
	skip(m1val - 1);
	puttl(header, curpag);
    }
    skip(m2val);
    lineno = m1val + m2val + 1;
}

// puttl: put out title line with optional page number
void puttl(Char * t, Int pageno)
{
    Char *pgnumstring;
    Char separator;
    Int i, l, m, n, pstlen;

    // Get the character that separates the three sections
    separator = *t;
    if (separator == '\0') {
	putchar('\n');
	return;
    }
    // Convert the page number into text
    pgnumstring = itoa(pageno);
    pstlen = Strlen(pgnumstring);

    // Move up past the quote and get length of first part
    // then prInt it out
    t++; l = titlen(t, separator, pstlen);
    t = partitle(t, separator, pgnumstring);
    if (*t == '\0') {
	putchar('\n');
	return;
    }
    // Get the length of the middle part
    t++; m = titlen(t, separator, pstlen);

    // Space out so we can prInt the middle out
    for (i = l; i < (rmval - m) / 2; i++)
	putchar(' ');
    t = partitle(t, separator, pgnumstring);
    if (*t == '\0') {
	putchar('\n');
	return;
    }
    if ((rmval - m) / 2 > l)
	m = m + (rmval - m) / 2;
    else
	m = m + l;
    // Get the length of the last bit
    t++; n = titlen(t, separator, pstlen);

    for (i = m; i < rmval - n; i++)
	putchar(' ');
    partitle(t, separator, pgnumstring);
    putchar('\n');
}

// PrInt out a part of the title. Return a pointer to the separator
Char *partitle(Char * t, Char sep, Char * pgnum)
{
    while (1) {
	// Stop when we reach the separator
	if (*t == sep || *t == '\0')
	    return (t);

	// PrInt out the character, or the page number on %
	if (*t == '%') {
	    while (*pgnum) {
		putchar(*pgnum);
		pgnum++;
	    }
	} else
	    putchar(*t);
	t++;
    }
    return (t);
}

//  skip: output n blank lines
void skip(Int n)
{
    Int i;

    for (i = 0; i < n; i++)
	putchar('\n');
    return;
}

// ctoi - convert string to int
Int ctoi(Char * in)
{
    Int k = 0;
    Int negval = 0;
    // Skip + and -, and set the negval
    if (*in == '+')
	in++;
    if (*in == '-') {
	negval = 1;
	in++;
    }

    // Read digits, convert to ints
    while (1) {
	if (*in < '0' || *in > '9')
	    break;
	// Move up old value and add on next digit
	k = 10 * k + *in - '0';
	in++;
    }

    // Turn into a negative value if required
    if (negval)
	return (0 - k);
    else
	return (k);
}

// Return the length of the string that c points at
Int Strlen(Char * c)
{
    Int n = 0;
    while (*c != '\0') {
	c++;
	n++;
    }
    return (n);
}

// Return the length of the word that c points at
// Also set the position of the first hypchar, or 0 if no hypchar
Int Wordlen(Char * c)
{
    Int n = 0;
    hyplen=0;
    while (*c != '\0' && *c != ' ' && *c != '\t') {
	if (*c == hypchar) {
	    if (hyplen==0) hyplen= n + 1;
	} else {
	    n++;
	}
	c++;
    }
    return (n);
}

// Given a character, return 1 if it can be underlined, 0 otherwise
Int is_underl(Char ch)
{
    if (ch == ' ' || ch == '\t' || ch == '!' || ch == ','
	|| ch == '.' || ch == ';' || ch == ':')
	return (0);
    return (1);
}

// Copy a string from src to dst
void Strcpy(Char * dst, Char * src)
{
    while (*src != '\0') {
	*dst = *src;
	dst++;
	src++;
    }
    *dst = '\0';
}

// Copy up to n characters from one string to another
// Like strncpy but we also deal with the hyphenation character
void Wordcopy(Char * dst, Char * src, Int n)
{
    while (*src != '\0' && n) {
	if (*src != hypchar) {
	  *dst = *src;
	  dst++;
	  n--;
	}
	src++;
    }
    *dst = '\0';
}

// Convert an integer into a string and return a pointer to it
Char *itoa(Int n)
{
    Char *digitptr;
    digitptr = tempbuf + 9;		// i.e digiptr= &tempbuf[9]
    *digitptr = '\0';
    digitptr--;

    while (n != 0) {
	*digitptr = (n % 10) + '0';	// Store a digit
	digitptr--;
	n = n / 10;
    }

    return (digitptr + 1);
}

// Given a title string just past the dividing character
// the dividing character in c, and the length of the
// page number in characters, return the length of the title
// Comes from Minix roff.
Int titlen(Char * t, Char c, Int k)
{
    Int q;
    q = 0;
    while (1) {
	if (*t == 0)
	    break;
	if (*t == c)
	    break;
	if (*t == '%')
	    q = q + k;
	else
	    q++;
	t++;
    }
    return (q);
}

// Find the minimum of two integers
Int min(Int a, Int b)
{
    if (a < b)
	return (a);
    else
	return (b);
}

// Put out a string with a newline
void Puts(Char * c)
{
    char ch;
    while (*c != '\0') {
	ch = *c;
	putchar(ch);
	c++;
    }
    putchar('\n');
}
