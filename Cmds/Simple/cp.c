/* cp - copy files		Author: Andy Tanenbaum */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define TRANSFER_UNIT    16384
char cpbuf[TRANSFER_UNIT];

void usage(void);
int cp_to_dir(int argc, char *argv[]);
int equal(char *s1, char *s2);
void stderr3(char *s1, char *s2, char *s3);
void copyfile(int fd1, int fd2, char *name);
int memcmp(char * b1, char * b2, int n);

int main(int argc, char *argv[])
{
  int fd1, fd2, m, s;
  struct stat sbuf, sbuf2;

  if (argc < 3) usage();

  /* Get the status of the last named file.  See if it is a directory. */
  s = stat(argv[argc - 1], &sbuf);
  m = sbuf.st_mode & S_IFMT;
  if (s >= 0 && m == S_IFDIR) {
	/* Last argument is a directory. */
	exit(cp_to_dir(argc, argv));
  } else if (argc > 3) {
	/* More than 2 arguments and last one is not a directory. */
	usage();
  } else if (s < 0 || m == S_IFREG || m == S_IFCHR || m == S_IFBLK) {
	/* Exactly two arguments.  Check for cp f1 f1. */
	if (equal(argv[1], argv[2])) {
		fprintf(stderr, "cp: cannot copy a file to itself\n");
		exit(-1);
	}

	/* Command is of the form cp f1 f2. */
	fd1 = open(argv[1], O_RDONLY);
	if (fd1 < 0) {
		stderr3("cannot open ", argv[1], "\n");
		exit(1);
	}
	fstat(fd1, &sbuf);
	m = sbuf.st_mode & S_IFMT;
	if (m == S_IFDIR) {
		stderr3("<", argv[1], "> directory\n");
		exit(1);
	}
	fd2 = creat(argv[2], sbuf.st_mode & 0777);
	if (fd2 < 0) {
		stderr3("cannot create ", argv[2], "\n");
		exit(2);
	}
	fstat(fd2, &sbuf2);
	copyfile(fd1, fd2, argv[2]);
  } else {
	stderr3("cannot copy to ", argv[2], "\n");
	exit(3);
  }
  exit(0);
}





int cp_to_dir(int argc, char *argv[])
{
  int i, mode, fd1, fd2, exit_status = 0;
  char dirname[256], *ptr, *dp;
  struct stat sbuf;

  for (i = 1; i < argc - 1; i++) {
	fd1 = open(argv[i], O_RDONLY);
	if (fd1 < 0) {
		stderr3("cannot open ", argv[i], "\n");
		exit_status = 1;
		continue;
	}
	ptr = argv[argc - 1];
	dp = dirname;
	while (*ptr != 0) *dp++ = *ptr++;

	*dp++ = '/';
	ptr = argv[i];

	/* Concatenate dir and file name in dirname buffer. */
	while (*ptr != 0) ptr++;/* go to end of file name */
	while (ptr > argv[i] && *ptr != '/') ptr--;	/* get last component */
	if (*ptr == '/') ptr++;
	while (*ptr != 0) *dp++ = *ptr++;
	*dp++ = 0;
	fstat(fd1, &sbuf);
	mode = sbuf.st_mode & S_IFMT;
	if (mode == S_IFDIR) {
		stderr3("<", argv[i], "> directory\n");
		exit_status = 1;
		close(fd1);
		continue;
	}
	fd2 = creat(dirname, sbuf.st_mode & 0777);
	if (fd2 < 0) {
		stderr3("cannot create ", dirname, "\n");
		exit_status = 2;
		close(fd1);
		continue;
	}
	copyfile(fd1, fd2, dirname);
  }
  return(exit_status);
}





void copyfile(int fd1, int fd2, char *name)
{
  int n, m, mode;
  struct stat sbuf;

  do {
	n = read(fd1, cpbuf, TRANSFER_UNIT);
	if (n < 0) {
		fprintf(stderr, "cp: read error\n");
		break;
	}
	if (n > 0) {
		m = write(fd2, cpbuf, n);
		if (m != n) {
			/* Write failed.  Don't keep truncated
			 * regular file. */
			perror("cp");
			fstat(fd2, &sbuf);	/* check for special files */
			mode = sbuf.st_mode & S_IFMT;
			if (mode == S_IFREG) unlink(name);
			exit(1);
		}
	}
  } while (n == TRANSFER_UNIT);
  close(fd1);
  close(fd2);
}

void usage(void)
{
  fprintf(stderr, "Usage:  cp f1 f2;  or  cp f1 ... fn d2\n");
  exit(-1);
}

int equal(char *s1, char *s2)
{
  struct stat sb1, sb2;

  /* Same file, different name? */
  stat(s1, &sb1);
  stat(s2, &sb2);
  if (memcmp((char *) & sb1, (char *) & sb2, sizeof(struct stat)) == 0)
	return(1);
  /* Same file, same name? */
  while (1) {
	if (*s1 == 0 && *s2 == 0) return(1);
	if (*s1 != *s2) return (0);
	if (*s1 == 0 || *s2 == 0) return (0);
	s1++;
	s2++;
  }
}

int match(char *s1, char *s2, int n)
{
  while (n--) {
	if (*s1++ != *s2++) return(0);
  }
  return(1);
}


void stderr3(char *s1, char *s2, char *s3)
{
  fprintf(stderr, "cp: ");
  fprintf(stderr, s1);
  fprintf(stderr, s2);
  fprintf(stderr, s3);
}


int memcmp(char * b1, char * b2, int n)
{
  while (n--) {
	if (*b1 != *b2) return((int) (*b1 - *b2));
	++b1;
	++b2;
  }
  return(0);
}
