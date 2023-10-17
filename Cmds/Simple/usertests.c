#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <xv6/param.h>
#include <xv6/types.h>
#include <xv6/stat.h>
#include <xv6/fs.h>
#include <xv6/fcntl.h>
#include <romcalls.h>

void cprintf(char *fmt, ...);

// Defines to ensure we use the xv6 versions
#define dup sys_dup
#define read sys_read
#define write sys_write
#define close sys_close
#define fstat sys_fstat
#define link sys_link
#define unlink sys_unlink
#define open sys_open
#define mkdir sys_mkdir
#define chdir sys_chdir
#define printf cprintf

char buf[8192];
char name[3];

void panic(char *str) {
  for (; *str; str++) romputc(*str);
  romputc('\n');
  exit(1);
}

// does chdir() call iput(p->cwd) in a transaction?
void iputtest(void) {
  printf("iput test\n");

  if (mkdir("iputdir") < 0) {
    printf("mkdir failed\n");
    exit(0);
  }
  if (chdir("iputdir") < 0) {
    printf("chdir iputdir failed\n");
    exit(0);
  }
  if (unlink("../iputdir") < 0) {
    printf("unlink ../iputdir failed\n");
    exit(0);
  }
  if (chdir("/") < 0) {
    printf("chdir / failed\n");
    exit(0);
  }
  printf("iput test ok\n");
}


// does the error path in open() for attempt to write a
// directory call iput() in a transaction?
// needs a hacked kernel that pauses just after the namei()
// call in sys_open():
//    if((ip = namei(path)) == 0)
//      return -1;
//    {
//      int i;
//      for(i = 0; i < 10000; i++)
//        yield();
//    }

// simple file system tests

void opentest(void) {
  int fd;

  printf("open test\n");
  fd = open("echo", 0);
  if (fd < 0) {
    printf("open echo failed!\n");
    exit(0);
  }
  close(fd);
  fd = open("doesnotexist", 0);
  if (fd >= 0) {
    printf("open doesnotexist succeeded!\n");
    exit(0);
  }
  printf("open test ok\n");
}

void writetest(void) {
  int fd;
  int i;

  printf("small file test\n");
  fd = open("small", O_CREATE | O_RDWR);
  if (fd >= 0) {
    printf("creat small succeeded; ok\n");
  } else {
    printf("error: creat small failed!\n");
    exit(0);
  }
  for (i = 0; i < 100; i++) {
    if (write(fd, "aaaaaaaaaa", 10) != 10) {
      printf("error: write aa %d new file failed\n", i);
      exit(0);
    }
    if (write(fd, "bbbbbbbbbb", 10) != 10) {
      printf("error: write bb %d new file failed\n", i);
      exit(0);
    }
  }
  printf("writes ok\n");
  close(fd);
  fd = open("small", O_RDONLY);
  if (fd >= 0) {
    printf("open small succeeded ok\n");
  } else {
    printf("error: open small failed!\n");
    exit(0);
  }
  i = read(fd, buf, 2000);
  if (i == 2000) {
    printf("read succeeded ok\n");
  } else {
    printf("read failed\n");
    exit(0);
  }
  close(fd);

  if (unlink("small") < 0) {
    printf("unlink small failed\n");
    exit(0);
  }
  printf("small file test ok\n");
}

void writetest1(void) {
  int i, fd, n;

  printf("big files test\n");

  fd = open("big", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("error: creat big failed!\n");
    exit(0);
  }

  for (i = 0; i < MAXFILE; i++) {
    ((int *) buf)[0] = i;
    if (write(fd, buf, 512) != 512) {
      printf("error: write big file failed: %i\n", i);
      exit(0);
    }
  }

  close(fd);

  fd = open("big", O_RDONLY);
  if (fd < 0) {
    printf("error: open big failed!\n");
    exit(0);
  }

  n = 0;
  for (;;) {
    i = read(fd, buf, 512);
    if (i == 0) {
      if (n == MAXFILE - 1) {
	printf("read only %d blocks from big", n);
	exit(0);
      }
      break;
    } else if (i != 512) {
      printf("read failed %d\n", i);
      exit(0);
    }
    if (((int *) buf)[0] != n) {
      printf("read content of block %d is %d\n", n, ((int *) buf)[0]);
      exit(0);
    }
    n++;
  }
  close(fd);
  if (unlink("big") < 0) {
    printf("unlink big failed\n");
    exit(0);
  }
  printf("big files ok\n");
}

void createtest(void) {
  int i, fd;

  printf("many creates, followed by unlink test\n");

  name[0] = 'a';
  name[2] = '\0';
  for (i = 0; i < 52; i++) {
    name[1] = (char)('0' + i);
    fd = open(name, O_CREATE | O_RDWR);
    close(fd);
  }
  name[0] = 'a';
  name[2] = '\0';
  for (i = 0; i < 52; i++) {
    name[1] = (char)('0' + i);
    unlink(name);
  }
  printf("many creates, followed by unlink; ok\n");
}

void dirtest(void) {
  printf("mkdir test\n");

  if (mkdir("dir0") < 0) {
    printf("mkdir failed\n");
    exit(0);
  }

  if (chdir("dir0") < 0) {
    printf("chdir dir0 failed\n");
    exit(0);
  }

  if (chdir("..") < 0) {
    printf("chdir .. failed\n");
    exit(0);
  }

  if (unlink("dir0") < 0) {
    printf("unlink dir0 failed\n");
    exit(0);
  }
  printf("mkdir test ok\n");
}

// More file system tests




// can I unlink a file and still read it?
void unlinkread(void) {
  int fd, fd1;

  printf("unlinkread test\n");
  fd = open("unlinkread", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("create unlinkread failed\n");
    exit(0);
  }
  write(fd, "hello", 5);
  close(fd);

  fd = open("unlinkread", O_RDWR);
  if (fd < 0) {
    printf("open unlinkread failed\n");
    exit(0);
  }
  if (unlink("unlinkread") != 0) {
    printf("unlink unlinkread failed\n");
    exit(0);
  }

  fd1 = open("unlinkread", O_CREATE | O_RDWR);
  write(fd1, "yyy", 3);
  close(fd1);

  if (read(fd, buf, sizeof(buf)) != 5) {
    printf("unlinkread read failed");
    exit(0);
  }
  if (buf[0] != 'h') {
    printf("unlinkread wrong data\n");
    exit(0);
  }
  if (write(fd, buf, 10) != 10) {
    printf("unlinkread write failed\n");
    exit(0);
  }
  close(fd);
  unlink("unlinkread");
  printf("unlinkread ok\n");
}

void linktest(void) {
  int fd;

  printf("linktest\n");

  unlink("lf1");
  unlink("lf2");

  fd = open("lf1", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("create lf1 failed\n");
    exit(0);
  }
  if (write(fd, "hello", 5) != 5) {
    printf("write lf1 failed\n");
    exit(0);
  }
  close(fd);

  if (link("lf1", "lf2") < 0) {
    printf("link lf1 lf2 failed\n");
    exit(0);
  }
  unlink("lf1");

  if (open("lf1", 0) >= 0) {
    printf("unlinked lf1 but it is still there!\n");
    exit(0);
  }

  fd = open("lf2", 0);
  if (fd < 0) {
    printf("open lf2 failed\n");
    exit(0);
  }
  if (read(fd, buf, sizeof(buf)) != 5) {
    printf("read lf2 failed\n");
    exit(0);
  }
  close(fd);

  if (link("lf2", "lf2") >= 0) {
    printf("link lf2 lf2 succeeded! oops\n");
    exit(0);
  }

  unlink("lf2");
  if (link("lf2", "lf1") >= 0) {
    printf("link non-existant succeeded! oops\n");
    exit(0);
  }

  if (link(".", "lf1") >= 0) {
    printf("link . lf1 succeeded! oops\n");
    exit(0);
  }

  printf("linktest ok\n");
}



// directory that uses indirect blocks
void bigdir(void) {
  int i, fd;
  char name[10];

  printf("bigdir test\n");
  unlink("bd");

  fd = open("bd", O_CREATE);
  if (fd < 0) {
    printf("bigdir create failed\n");
    exit(0);
  }
  close(fd);

  for (i = 0; i < 500; i++) {
    name[0] = 'x';
    name[1] = (char)('0' + (i / 64));
    name[2] = (char)('0' + (i % 64));
    name[3] = '\0';
    if (link("bd", name) != 0) {
      printf("bigdir link failed\n");
      exit(0);
    }
  }

  unlink("bd");
  for (i = 0; i < 500; i++) {
    name[0] = 'x';
    name[1] = (char)('0' + (i / 64));
    name[2] = (char)('0' + (i % 64));
    name[3] = '\0';
    if (unlink(name) != 0) {
      printf("bigdir unlink failed");
      exit(0);
    }
  }

  printf("bigdir ok\n");
}

void subdir(void) {
  int fd, cc;

  printf("subdir test\n");

  unlink("ff");
  if (mkdir("dd") != 0) {
    printf("subdir mkdir dd failed\n");
    exit(0);
  }

  fd = open("dd/ff", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("create dd/ff failed\n");
    exit(0);
  }
  write(fd, "ff", 2);
  close(fd);

  if (unlink("dd") >= 0) {
    printf("unlink dd (non-empty dir) succeeded!\n");
    exit(0);
  }

  if (mkdir("/dd/dd") != 0) {
    printf("subdir mkdir dd/dd failed\n");
    exit(0);
  }

  fd = open("dd/dd/ff", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("create dd/dd/ff failed\n");
    exit(0);
  }
  write(fd, "FF", 2);
  close(fd);

  fd = open("dd/dd/../ff", 0);
  if (fd < 0) {
    printf("open dd/dd/../ff failed\n");
    exit(0);
  }
  cc = read(fd, buf, sizeof(buf));
  if (cc != 2 || buf[0] != 'f') {
    printf("dd/dd/../ff wrong content\n");
    exit(0);
  }
  close(fd);

  if (link("dd/dd/ff", "dd/dd/ffff") != 0) {
    printf("link dd/dd/ff dd/dd/ffff failed\n");
    exit(0);
  }

  if (unlink("dd/dd/ff") != 0) {
    printf("unlink dd/dd/ff failed\n");
    exit(0);
  }
  if (open("dd/dd/ff", O_RDONLY) >= 0) {
    printf("open (unlinked) dd/dd/ff succeeded\n");
    exit(0);
  }

  if (chdir("dd") != 0) {
    printf("chdir dd failed\n");
    exit(0);
  }
  if (chdir("dd/../../dd") != 0) {
    printf("chdir dd/../../dd failed\n");
    exit(0);
  }
  if (chdir("dd/../../../dd") != 0) {
    printf("chdir dd/../../dd failed\n");
    exit(0);
  }
  if (chdir("./..") != 0) {
    printf("chdir ./.. failed\n");
    exit(0);
  }

  fd = open("dd/dd/ffff", 0);
  if (fd < 0) {
    printf("open dd/dd/ffff failed\n");
    exit(0);
  }
  if (read(fd, buf, sizeof(buf)) != 2) {
    printf("read dd/dd/ffff wrong len\n");
    exit(0);
  }
  close(fd);

  if (open("dd/dd/ff", O_RDONLY) >= 0) {
    printf("open (unlinked) dd/dd/ff succeeded!\n");
    exit(0);
  }

  if (open("dd/ff/ff", O_CREATE | O_RDWR) >= 0) {
    printf("create dd/ff/ff succeeded!\n");
    exit(0);
  }
  if (open("dd/xx/ff", O_CREATE | O_RDWR) >= 0) {
    printf("create dd/xx/ff succeeded!\n");
    exit(0);
  }
  if (open("dd", O_CREATE) >= 0) {
    printf("create dd succeeded!\n");
    exit(0);
  }
  if (open("dd", O_RDWR) >= 0) {
    printf("open dd rdwr succeeded!\n");
    exit(0);
  }
  if (open("dd", O_WRONLY) >= 0) {
    printf("open dd wronly succeeded!\n");
    exit(0);
  }
  if (link("dd/ff/ff", "dd/dd/xx") == 0) {
    printf("link dd/ff/ff dd/dd/xx succeeded!\n");
    exit(0);
  }
  if (link("dd/xx/ff", "dd/dd/xx") == 0) {
    printf("link dd/xx/ff dd/dd/xx succeeded!\n");
    exit(0);
  }
  if (link("dd/ff", "dd/dd/ffff") == 0) {
    printf("link dd/ff dd/dd/ffff succeeded!\n");
    exit(0);
  }
  if (mkdir("dd/ff/ff") == 0) {
    printf("mkdir dd/ff/ff succeeded!\n");
    exit(0);
  }
  if (mkdir("dd/xx/ff") == 0) {
    printf("mkdir dd/xx/ff succeeded!\n");
    exit(0);
  }
  if (mkdir("dd/dd/ffff") == 0) {
    printf("mkdir dd/dd/ffff succeeded!\n");
    exit(0);
  }
  if (unlink("dd/xx/ff") == 0) {
    printf("unlink dd/xx/ff succeeded!\n");
    exit(0);
  }
  if (unlink("dd/ff/ff") == 0) {
    printf("unlink dd/ff/ff succeeded!\n");
    exit(0);
  }
  if (chdir("dd/ff") == 0) {
    printf("chdir dd/ff succeeded!\n");
    exit(0);
  }
  if (chdir("dd/xx") == 0) {
    printf("chdir dd/xx succeeded!\n");
    exit(0);
  }

  if (unlink("dd/dd/ffff") != 0) {
    printf("unlink dd/dd/ff failed\n");
    exit(0);
  }
  if (unlink("dd/ff") != 0) {
    printf("unlink dd/ff failed\n");
    exit(0);
  }
  if (unlink("dd") == 0) {
    printf("unlink non-empty dd succeeded!\n");
    exit(0);
  }
  if (unlink("dd/dd") < 0) {
    printf("unlink dd/dd failed\n");
    exit(0);
  }
  if (unlink("dd") < 0) {
    printf("unlink dd failed\n");
    exit(0);
  }

  printf("subdir ok\n");
}

// test writes that are larger than the log.
void bigwrite(void) {
  int fd, sz;

  printf("bigwrite test\n");

  unlink("bigwrite");
  for (sz = 499; sz < 12 * 512; sz += 471) {
    fd = open("bigwrite", O_CREATE | O_RDWR);
    if (fd < 0) {
      printf("cannot create bigwrite\n");
      exit(0);
    }
    int i;
    for (i = 0; i < 2; i++) {
      int cc = write(fd, buf, sz);
      if (cc != sz) {
	printf("write(%d) ret %d\n", sz, cc);
	exit(0);
      }
    }
    close(fd);
    unlink("bigwrite");
  }

  printf("bigwrite ok\n");
}

void bigfile(void) {
  int fd, i, total, cc;

  printf("bigfile test\n");

  unlink("bigfile");
  fd = open("bigfile", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("cannot create bigfile");
    exit(0);
  }
  for (i = 0; i < 20; i++) {
    memset(buf, i, 600);
    if (write(fd, buf, 600) != 600) {
      printf("write bigfile failed\n");
      exit(0);
    }
  }
  close(fd);

  fd = open("bigfile", 0);
  if (fd < 0) {
    printf("cannot open bigfile\n");
    exit(0);
  }
  total = 0;
  for (i = 0;; i++) {
    cc = read(fd, buf, 300);
    if (cc < 0) {
      printf("read bigfile failed\n");
      exit(0);
    }
    if (cc == 0)
      break;
    if (cc != 300) {
      printf("short read bigfile\n");
      exit(0);
    }
    if (buf[0] != i / 2 || buf[299] != i / 2) {
      printf("read bigfile wrong data\n");
      exit(0);
    }
    total += cc;
  }
  close(fd);
  if (total != 20 * 600) {
    printf("read bigfile wrong total\n");
    exit(0);
  }
  unlink("bigfile");

  printf("bigfile test ok\n");
}

void fourteen(void) {
  int fd;

  // DIRSIZ is 14.
  printf("fourteen test\n");

  if (mkdir("12345678901234") != 0) {
    printf("mkdir 12345678901234 failed\n");
    exit(0);
  }
  if (mkdir("12345678901234/123456789012345") != 0) {
    printf("mkdir 12345678901234/123456789012345 failed\n");
    exit(0);
  }
  fd = open("123456789012345/123456789012345/123456789012345", O_CREATE);
  if (fd < 0) {
    printf("create 123456789012345/123456789012345/123456789012345 failed\n");
    exit(0);
  }
  close(fd);
  fd = open("12345678901234/12345678901234/12345678901234", 0);
  if (fd < 0) {
    printf("open 12345678901234/12345678901234/12345678901234 failed\n");
    exit(0);
  }
  close(fd);

  if (mkdir("12345678901234/12345678901234") == 0) {
    printf("mkdir 12345678901234/12345678901234 succeeded!\n");
    exit(0);
  }
  if (mkdir("123456789012345/12345678901234") == 0) {
    printf("mkdir 12345678901234/123456789012345 succeeded!\n");
    exit(0);
  }

  printf("fourteen ok\n");
}

void rmdot(void) {
  printf("rmdot test\n");
  if (mkdir("dots") != 0) {
    printf("mkdir dots failed\n");
    exit(0);
  }
  if (chdir("dots") != 0) {
    printf("chdir dots failed\n");
    exit(0);
  }
  if (unlink(".") == 0) {
    printf("rm . worked!\n");
    exit(0);
  }
  if (unlink("..") == 0) {
    printf("rm .. worked!\n");
    exit(0);
  }
  if (chdir("/") != 0) {
    printf("chdir / failed\n");
    exit(0);
  }
  if (unlink("dots/.") == 0) {
    printf("unlink dots/. worked!\n");
    exit(0);
  }
  if (unlink("dots/..") == 0) {
    printf("unlink dots/.. worked!\n");
    exit(0);
  }
  if (unlink("dots") != 0) {
    printf("unlink dots failed!\n");
    exit(0);
  }
  printf("rmdot ok\n");
}

void dirfile(void) {
  int fd;

  printf("dir vs file\n");

  fd = open("dirfile", O_CREATE);
  if (fd < 0) {
    printf("create dirfile failed\n");
    exit(0);
  }
  close(fd);
  if (chdir("dirfile") == 0) {
    printf("chdir dirfile succeeded!\n");
    exit(0);
  }
  fd = open("dirfile/xx", 0);
  if (fd >= 0) {
    printf("create dirfile/xx succeeded!\n");
    exit(0);
  }
  fd = open("dirfile/xx", O_CREATE);
  if (fd >= 0) {
    printf("create dirfile/xx succeeded!\n");
    exit(0);
  }
  if (mkdir("dirfile/xx") == 0) {
    printf("mkdir dirfile/xx succeeded!\n");
    exit(0);
  }
  if (unlink("dirfile/xx") == 0) {
    printf("unlink dirfile/xx succeeded!\n");
    exit(0);
  }
  if (link("README", "dirfile/xx") == 0) {
    printf("link to dirfile/xx succeeded!\n");
    exit(0);
  }
  if (unlink("dirfile") != 0) {
    printf("unlink dirfile failed!\n");
    exit(0);
  }

  fd = open(".", O_RDWR);
  if (fd >= 0) {
    printf("open . for writing succeeded!\n");
    exit(0);
  }
  fd = open(".", 0);
  if (write(fd, "x", 1) > 0) {
    printf("write . succeeded!\n");
    exit(0);
  }
  close(fd);

  printf("dir vs file OK\n");
}

// test that iput() is called at the end of _namei()
void iref(void) {
  int i, fd;

  printf("empty file name\n");

  // the 50 is NINODE
  for (i = 0; i < 50 + 1; i++) {
    if (mkdir("irefd") != 0) {
      printf("mkdir irefd failed\n");
      exit(0);
    }
    if (chdir("irefd") != 0) {
      printf("chdir irefd failed\n");
      exit(0);
    }

    mkdir("");
    link("README", "");
    fd = open("", O_CREATE);
    if (fd >= 0)
      close(fd);
    fd = open("xx", O_CREATE);
    if (fd >= 0)
      close(fd);
    unlink("xx");
  }

  chdir("/");
  printf("empty file name OK\n");
}


// what happens when the file system runs out of blocks?
// answer: balloc panics, so this test is not useful.
void fsfull() {
  int nfiles;
  int fsblocks = 0;

  printf("fsfull test\n");

  for (nfiles = 0;; nfiles++) {
    char name[64];
    name[0] = 'f';
    name[1] = (char)('0' + nfiles / 1000);
    name[2] = (char)('0' + (nfiles % 1000) / 100);
    name[3] = (char)('0' + (nfiles % 100) / 10);
    name[4] = (char)('0' + (nfiles % 10));
    name[5] = '\0';
    printf("writing %s\n", name);
    int fd = open(name, O_CREATE | O_RDWR);
    if (fd < 0) {
      printf("open %s failed\n", name);
      break;
    }
    int total = 0;
    while (1) {
      int cc = write(fd, buf, 512);
      if (cc < 512)
	break;
      total += cc;
      fsblocks++;
    }
    printf("wrote %d bytes\n", total);
    close(fd);
    if (total == 0)
      break;
  }

  while (nfiles >= 0) {
    char name[64];
    name[0] = 'f';
    name[1] = (char)('0' + nfiles / 1000);
    name[2] = (char)('0' + (nfiles % 1000) / 100);
    name[3] = (char)('0' + (nfiles % 100) / 10);
    name[4] = (char)('0' + (nfiles % 10));
    name[5] = '\0';
    unlink(name);
    nfiles--;
  }

  printf("fsfull test finished\n");
}

int main(int argc, char *argv[]) {
  printf("usertests starting\n");
  printf("sizeof struct dinode %ld\n", sizeof(struct dinode));

  if (open("usertests.ran", 0) >= 0) {
    printf("already ran user tests -- rebuild fs.img\n");
    exit(0);
  }
  close(open("usertests.ran", O_CREATE));


  bigwrite();
  opentest();
  writetest();
  // writetest1();		// The only one that fails now
  createtest();
  iputtest();
  rmdot();
  fourteen();
  bigfile();
  subdir();
  linktest();
  unlinkread();
  dirfile();
  iref();
  bigdir();			// slow

  exit(0);
  return(0);
}
