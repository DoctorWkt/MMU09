#ifndef _FCNTL_H
#define _FCNTL_H

#include <sys/types.h>

#define O_RDONLY  0x0000
#define O_WRONLY  0x0001
#define O_RDWR    0x0002
#define O_CREAT   0x0200
#define O_APPEND  0x0008
#define O_TRUNC   0x0400

// These ones not implemented yet

#define O_ACCMODE	3
#define O_SYNC		8
#define O_NDELAY	16
#define O_EXCL		512
#define O_NOCTTY	2048
#define O_CLOEXEC	4096
#define O_SYMLINK	8192	/* Not supported in kernel yet */

#define FD_CLOEXEC	O_CLOEXEC

#define O_BINARY	0	/* not used in Fuzix */

#define O_NONBLOCK	O_NDELAY

#define F_GETFL		0
#define F_SETFL		1
#define F_GETFD		2
#define F_SETFD		3
#define F_DUPFD		4
/* Not current implemented in Fuzix */
#define F_GETLK		5
#define F_SETLK		6
#define F_SETLKW	7

typedef struct flock {
   short l_type;
   short l_whence;
   off_t l_start;
   off_t l_len;
   pid_t l_pid;
} flock_t;

#define FNDELAY		O_NDELAY

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#endif

