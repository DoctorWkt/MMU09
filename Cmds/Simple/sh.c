#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <xv6/param.h>
#include <unistd.h>
#include <fcntl.h>
#include <romcalls.h>
#include <readline/readline.h>
void cprintf(char *fmt, ...);

#define MAXARG 20		// Maximum number of arguments
#define MAXLIN 100		// Maximum line size

int realargc;			// Real argc after parsing the command


// Close any open file descriptors on exit
void myexit(int x) {
  for (int fd = 0; fd < NFILE; fd++)
    sys_close(fd);
  exit(0);
}

// Return true if the name matches the given pattern.
// Only ? and * are recognised. This code from Russ Cox:
// https://research.swtch.com/glob
int match(char *pattern, char *name) {
  int px = 0, nx = 0;
  int nextpx = 0, nextnx = 0;
  int plen = strlen(pattern);
  int nlen = strlen(name);
  char c;

  for (; px < plen || nx < nlen;) {
    if (px < plen) {
      c = pattern[px];

      switch (c) {
      case '?':		// Single character wildcard
	if (nx < nlen) {
	  px++;
	  nx++;
	  continue;
	}
	break;

      case '*':		// Zero or more character wildcard
	// Try to match at nx. If that
	// doesn't work out, restart at
	// nx+1 next.
	nextpx = px;
	nextnx = nx + 1;
	px++;
	continue;

      default:			// Ordinary character
	if (nx < nlen && name[nx] == c) {
	  px++;
	  nx++;
	  continue;
	}
      }
    }

    // Mismatch, maybe restart
    if (0 < nextnx && nextnx <= nlen) {
      px = nextpx;
      nx = nextnx;
      continue;
    }
    return (0);
  }
  // Matched all of pattern to all of name. Success.
  return (1);
}

// Deal with redirections and pipelines in the current argv list
void redirect(int argc, char *argv[]) {
  char seekbuf[MAXLIN];
  int fd, len;

  // Assume we won't have any redirection
  realargc = argc;

  // Walk the argument list and process redirections: <, >, >> and 2> only.
  // We go to the 2nd-last argument so we can still process the next one.
  for (int i = 0; i < argc - 1; i++) {

    // Stdout redirected to a file
    if (!strcmp(argv[i], ">")) {

      // Open the file. 
      if ((fd = sys_open(argv[i + 1], O_CREAT | O_TRUNC | O_WRONLY)) == -1) {
	cprintf("Cannot open %s\n", argv[i + 1]);
	exit(1);
      }

      // Close stdout and dup the fd down to stdout, then close the fd
      sys_close(1);
      sys_dup(fd);
      sys_close(fd);

      // Set the arg count to before this token. Skip the next token
      if (realargc == argc)
	realargc = i;
      i++;
    }

    // Stdout redirected to a file, appending
    if (!strcmp(argv[i], ">>")) {

      // Open the file for reading and writing, so we can lseek on it.
      if ((fd = sys_open(argv[i + 1], O_WRONLY | O_APPEND)) == -1) {
	cprintf("Cannot open %s\n", argv[i + 1]);
	exit(1);
      }

      // Close stdout and dup the fd down to stdout, then close the fd
      sys_close(1);
      sys_dup(fd);
      sys_close(fd);

      // Read to the end of the file
      // while ((len=sys_read(1, seekbuf, MAXLIN))>0) ;

      // Set the arg count to before this token. Skip the next token
      if (realargc == argc)
	realargc = i;
      i++;
    }

    // Stdin redirected from a file
    if (!strcmp(argv[i], "<")) {

      // Open the file.
      if ((fd = sys_open(argv[i + 1], O_RDONLY)) == -1) {
	cprintf("Cannot open %s\n", argv[i + 1]);
	myexit(1);
      }

      // Close stdin and dup the fd down to stdin, then close the fd
      sys_close(0);
      sys_dup(fd);
      sys_close(fd);

      // Set the arg count to before this token. Skip the next token
      if (realargc == argc)
	realargc = i;
      i++;
    }

    // Stderr redirected to a file
    if (!strcmp(argv[i], "2>")) {

      // Open the file.
      if ((fd = sys_open(argv[i + 1], O_CREAT | O_TRUNC | O_WRONLY)) == -1) {
	cprintf("Cannot open %s\n", argv[i + 1]);
	myexit(1);
      }

      // Close stderr and dup the fd down to stderr, then close the fd
      sys_close(2);
      sys_dup(fd);
      sys_close(fd);

      // Set the arg count to before this token. Skip the next token
      if (realargc == argc)
	realargc = i;
      i++;
    }

    // A pipeline
    if (!strcmp(argv[i], "|")) {

      // Open a temporary file for the output of this command
      if ((fd = sys_open("/.pipedata", O_CREAT | O_TRUNC | O_WRONLY)) == -1) {
	cprintf("Cannot open /.pipedata\n");
	myexit(1);
      }

      // Close stdout and dup the fd down to stdout, then close the fd
      sys_close(1);
      sys_dup(fd);
      sys_close(fd);

      // Open a file to store the rest of the pipeline command
      if ((fd = sys_open("/.pipecmd", O_CREAT | O_TRUNC | O_WRONLY)) == -1) {
	cprintf("Cannot open /.pipecmd\n");
	myexit(1);
      }

      // Set the arg count to before this token.
      if (realargc == argc)
	realargc = i;

      // Write the rest of the pipeline command to the file
      for (i = i + 1; i < argc; i++) {
	write(fd, argv[i], strlen(argv[i]));
	write(fd, " ", 1);
      }

      // Save the file and return now as there's nothing left to process
      close(fd);
      return;
    }
  }
}

// Parse the given line, generating argv
int parse(char *buf, char *argv[]) {
  int wordc;
  char *wordlist[MAXARG + 1];	// List of words after metachar expansion
  int argc = 0;
  int end;
  DIR *D;
  struct dirent *dent;
  char *cmdp = buf;

  // Split the line up into non-whitespace arguments or operators
  for (wordc = 0; wordc < MAXARG; wordc++) {

    // Get an argument. If none left, break the loop
    if ((wordlist[wordc] = strtok(cmdp, " \t\r\n")) == NULL)
      break;

    // Tell strtok to keep going
    cmdp = NULL;
  }

  // No arguments, just restart the shell
  if (wordc == 0)
    myexit(0);

  // Copy the words into the argv list for now
  for (int i = 0; i < wordc && argc < MAXARG; i++) {

    // If the word starts with a single or double quote, trim off the
    // characters at each end and add to the argv list
    if ((wordlist[i][0] == '\'') || (wordlist[i][0] == '"')) {
      end = strlen(wordlist[i]);
      wordlist[i][end - 1] = 0;
      argv[argc++] = &wordlist[i][1];
      continue;
    }

    // See if the word contains a pattern to match on, i.e. a '*' or '?'
    if ((index(wordlist[i], '?') != NULL)
	|| (index(wordlist[i], '*') != NULL)) {

      // For now we can only pattern match in this directory
      if (index(wordlist[i], '/') != NULL) {
	cprintf("Pattern matching only in this directory, sorry\n");
	myexit(1);
      }

      // Search the current directory for any matches
      D = opendir(".");
      if (D == NULL) {
	cprintf("Can't opendir .\n");
	myexit(1);
      }

      // Process each entry
      while ((dent = readdir(D)) != NULL) {

	// Skip empty directory entries
	if (dent->d_name[0] == '\0')
	  continue;

	// cprintf("About to match %s against %s\n", patternlist[i], dent->d_name);

	// If there's a match, add the filename to the argv list
	if (match(wordlist[i], dent->d_name)) {
	  argv[argc++] = dent->d_name;
	}
      }
      closedir(D);
      free(D);
      continue;
    }

    // Not a pattern or quoted word, add the word to the argv list
    argv[argc++] = wordlist[i];
  }

  return (argc);
}

int main() {
  int i, fd, pid, wstatus;
  int argc;
  char *buf = NULL;
  char *argv[MAXARG + 1];	// The argument list
  char binbuf[MAXLIN];		// Used to prepend "/bin/" to commands

  while (1) {
    // Use readline() to get the input line
    buf = readline("$ ");

    // Parse the input line, generating argv
    argc = parse(buf, argv);

    // See if the first argument is cd. If so, do the chdir and
    // then go back to the top of the loop for the next command
    if (!strcmp(argv[0], "cd")) {
      if (argc == 2) {
	if (sys_chdir(argv[1]) == -1)
	  cprintf("Cannot cd to %s\n", argv[1]);
      }
      continue;
    }

    // Try to open the argument.
    if ((fd = open(argv[0], O_RDONLY)) == -1) {
      // No such luck. Try putting "/bin/" on the front
      strcpy(binbuf, "/bin/");
      strcat(binbuf, argv[0]);
      if ((fd = open(binbuf, O_RDONLY)) == -1) {
	cprintf("%s: no such command\n", argv[0]);
	continue;
      }
      argv[0] = binbuf;
    }

    // See if it's an executable: always start with 0x3406
    read(fd, &i, 2);
    if (i != 0x3406) {
      cprintf("%s: not an executable\n", argv[0]);
      close(fd); continue;
    }
    close(fd);

    // Deal with any redirections
    redirect(argc, argv);

    // No arguments, loop back
    if (argc==0) continue;

    // Put a NULL at the end of the argv[] array
    argv[realargc] = NULL;

#if 0
    // Debug
    cprintf("sh argc %d\n", realargc);
    for (i=0; i<realargc; i++)
      cprintf("sh argv[%d] %s 0x%x\n", i, argv[i], argv[i]);
#endif

    // Now fork, start the new program and wait for it to exit
    pid= fork();
    switch (pid) {
      case 0:				// The child
        exec(argv[0], argv);
	cprintf("Unable to exec %s\n", argv[0]);
	exit(0);

      case -1:
	cprintf("Unable to fork()\n");
	exit(0);

      default:
	pid= wait(&wstatus);
    }
  }

  return (0);	// Should never be used
}
