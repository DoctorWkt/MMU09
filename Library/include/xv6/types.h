// The include files from FUZIX define uint and uchar already
typedef unsigned short ushort;
#ifdef __linux__

typedef unsigned int uint;
typedef unsigned char uchar;
typedef short Int;
typedef unsigned short Uint;
typedef int xvoff_t;
typedef unsigned short xvblk_t;
typedef unsigned short xvino_t;

#else

typedef long xvoff_t;
typedef unsigned int xvblk_t;
typedef unsigned int xvino_t;
typedef short Int;
typedef unsigned short Uint;

#endif
