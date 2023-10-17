#ifndef _DEV_NET_NET_NATIVE_H
#define _DEV_NET_NET_NATIVE_H

/* These are by socket and each one is

   [RX.0][RX.1]..[RX.n][TX.0][TX.1]...[TX.n]

*/

#define NSOCKBUF	4	/* 4 buffers per socket */
#define RINGSIZ		(1024 * NSOCKBUF)

/* The usable space will be lower once the headers are added for addressing */
#define TXPKTSIZ	1024
#define RXPKTSIZ	1024

#define SOCKBUFOFF	(2 * RINGSIZ)
#define RXBUFOFF	RINGSIZ

/* Total size is thus 8K * sockets - typically 64K for the file */


struct sockdata {
	void *socket;
	uint8_t ret;
	uint8_t event;		/* Waiting events to go to user space */
#define NEV_STATE	1
#define NEV_READ	2
#define NEV_WRITE	4
#define NEV_MASK	7
#define NEVW_STATE	128
	uint8_t newstate;	/* Requested new state */
	uint8_t lcn;		/* Logical channel */
	uint16_t rlen[NSOCKBUF];	/* TCP uses 0 as total space */
	uint16_t rbuf;
	uint16_t rnext;
	uint16_t tlen[NSOCKBUF];	/* Not used by TCP */
	uint16_t tbuf;		/* Next transmit buffer (pointer for tcp) */
	uint16_t tnext;		/* Buffers of room (bytes if TCP) */
};

struct sockmsg {
	struct socket s;
	struct sockdata sd;
};

#define NE_NEWSTATE	1
#define NE_EVENT	2
#define NE_SETADDR	3
#define NE_INIT		4
#define NE_ROOM		5
#define NE_DATA		6
#define NE_SHUTR        7
#define NE_RESET        8
#define NE_UNHOOK	9

#define NE_SETADDR_SRC	0
#define NE_SETADDR_DST	1

struct netevent {
	uint8_t socket;
	uint8_t event;
	uint8_t ret;
	uint16_t data;
	union {
		uint16_t rlen[NSOCKBUF];
		struct ksockaddr addr;
	} info;
};

#define NET_INIT_BFD	0x4401
#define NET_INIT_BMEM	0x4402
#define NET_MAC		0x4403
#define NET_MTU		0x4404
#define NET_IPADDR	0x4405
#define NET_MASK	0x4406
#define NET_GATEWAY	0x4407

int netdev_write(void);
int netdev_read(uint8_t flag);
int netdev_ioctl(uint16_t request, char *data);
int netdev_close(void);


#endif
