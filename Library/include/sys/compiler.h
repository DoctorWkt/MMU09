#ifndef __SYS_COMPILER_H
#define __SYS_COMPILER_H
#if defined(__GNUC__)
#define __packed __attribute((packed))
#else
#define __packed
#endif
#endif

