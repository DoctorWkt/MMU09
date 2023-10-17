CC= vc '+mmu09'
CFLAGS= -O2 -nostdlib
ASM=vasm6809_std
ASMFLAGS=-quiet -nowarn=62 -opt-branch -opt-offset -Fvobj

OBJS= asrl.o asr.o div.o divs.o ldivs.o ldivu.o lmods.o lmul.o \
lsll.o lsl.o lsrl.o lsr.o mods.o mul.o ldiv.o \
a64l.o abort.o abs.o accept.o __argv.o asctime.o \
assert.o atexit.o atoi.o atoi_small.o atol.o bcmp.o bcopy.o bind.o bsearch.o \
bzero.o cfmakeraw.o cfree.o cfspeed.o clock.o clock_getres.o \
clock_gettime.o clock_settime.o confstr.o \
connect.o creat.o crypt.o ctermid.o ctime.o ctime_r.o cuserid.o err.o \
cprintf.o errno.o error.o exit.o fclose.o fdatasync.o \
fflush.o fgetc.o fgetgrent.o fgetpos.o fgetpwent.o fgets.o fopen.o \
fprintf.o fputc.o fputs.o fread.o fscanf.o fsetpos.o fsync.o \
ftell.o fwrite.o getcwd.o getenv.o __getgrent.o getgrgid.o getgrnam.o \
gethostname.o getloadavg.o getlogin.o getopt.o getpass.o getpeername.o \
getpw.o __getpwent.o getpwnam.o getpwuid.o gets.o getsockname.o \
gettimeofday.o getw.o gmtime.o gmtime_r.o grent.o index.o \
initgroups.o isalnum.o isalpha.o isascii.o isatty.o isblank.o iscntrl.o \
isdigit.o isgraph.o islower.o isprint.o ispunct.o isspace.o isupper.o \
isxdigit.o l64a.o labs.o libclose.o libintl.o libload.o libopen.o \
libopenfd.o listen.o localtim.o localtim_r.o lrand48.o lsearch.o \
lstat.o ltoa.o ltostr.o malloc.o memccpy.o memchr.o memcmp.o memcpy.o \
memmove.o memset.o mkfifo.o mkstemps.o \
opendir.o pathconf.o pause.o perror.o printf.o putchar.o \
putchar_wrapper.o putenv.o putgetch.o putpwent.o putw.o pwent.o qsort.o \
raise.o rand.o readline.o readline_fn.o readlink.o \
recv.o recvfrom.o regerror.o regexp.o regsub.o remount.o remove.o \
romcalls.o revoke.o rewind.o rindex.o sbrk.o scanf.o \
setbuffer.o setenv.o setjmp.o setlocale.o \
settimeofday.o setvbuf.o sgetl.o shutdown.o __signgam.o socket.o \
sprintf.o sputl.o sscanf.o stat.o statvfs.o stdio0.o stime.o strcasecmp.o \
strcasestr.o strcat.o strchr.o strcmp.o strcoll.o strcpy.o strcspn.o \
strdup.o strndup.o stricmp.o strlcpy.o strlen.o strncasecmp.o strncat.o strncmp.o \
strncpy.o strnicmp.o strnlen.o strpbrk.o strrchr.o strsep.o strsignal.o \
strspn.o strstr.o strtok.o strtok_r.o strtol.o strxfrm.o swab.o swapon.o \
sysconf.o tcdrain.o tcflow.o tcflush.o tcgetattr.o tcsetattr.o \
termcap.o tfind.o tgetent.o time.o tmpfile.o \
tmpnam.o tolower.o toupper.o tsearch.o ttyname.o tzset.o umount.o ungetc.o \
utent.o utimes.o utsname.o varargs.o vfprintf.o vfprintf_m.o \
vfscanf.o vfscanf_m.o vprintf.o vscanf.o vsscanf.o xitoa.o

libmmu09.a: ${OBJS}
	rm -f libmmu09.a
	ar vr libmmu09.a ${OBJS}

all: ${OBJS}

.c.o:
	$(CC) -c $(CFLAGS) $<

.s.o:
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -f ${OBJS} libmmu09.a
