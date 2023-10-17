#ifndef __DIRENT_H
#define __DIRENT_H
#include <xv6/types.h>
#include <xv6/fs.h>

struct dirent {
        u_int   d_fileno;               /* file number of entry */
        u_short d_reclen;               /* length of this record */
        u_short d_namlen;               /* length of string in d_name */
        char    d_name[DIRSIZ + 1];     /* name must be no longer than this */
};

/* structure describing an open directory. */
typedef struct _dirdesc {
        int     dd_fd;          /* file descriptor associated with directory */
        long    dd_loc;         /* offset in current buffer */
        long    dd_size;        /* amount of data returned by getdirentries */
        char    *dd_buf;        /* data buffer */
        int     dd_len;         /* size of data buffer */
        long    dd_seek;        /* magic cookie returned by getdirentries */
        void    *dd_ddloc;      /* Linked list of ddloc structs for telldir/seekdir */
} DIR;

#define dirfd(dirp)     ((dirp)->dd_fd)

int closedir(DIR *dirp);
DIR *opendir(const char *dirname);
struct dirent *readdir(DIR *dirp);
void rewinddir(DIR *dirp);

#endif /* dirent.h  */
