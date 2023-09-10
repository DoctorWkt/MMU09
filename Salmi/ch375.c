// Simple simulation of the CH375 USB storage controller.
// Just enough to read/write blocks from a USB drive image.
// (c) 2023 Warren Toomey, GPL3.

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// List of known commands
#define GET_IC_VER	0x01
#define RESET_ALL	0x05
#define CHECK_EXIST	0x06
#define SET_USB_MODE	0x15
#define TEST_CONNECTION	0x16
#define GET_STATUS	0x22
#define RD_USB_DATA	0x28
#define WR_USB_DATA	0x2B
#define WR_USB_DATA1	0xFF		// Not a real cmd, to tell us that
					// we already have the maxcnt
#define DISK_INIT	0x51
#define DISK_SIZE	0x53
#define DISK_READ	0x54
#define DISK_RD_GO	0x55
#define DISK_WRITE	0x56
#define DISK_WR_GO	0x57

// List of known CH375 status
#define USB_INT_SUCCESS		0x14
#define USB_INT_CONNECT		0x15
#define USB_INT_DISK_READ	0x1D
#define USB_INT_DISK_WRITE	0x1E
#define USB_INT_DISK_ERR	0x1F

// Current CH375 status
// and the previous command
static unsigned char status = 0;
static unsigned char prevcmd = 0;

// File which holds the USB disk image
extern char *ch375file;
static FILE *disk;

// We receive and transmit data from this buffer.
// The index is the position of the next byte to send/receive.
// The count is the number of bytes remaining to send.
// The max count is what the 6809 told us it would send.
static unsigned char buf[64];
static unsigned char bufindex = 0;
static unsigned int bufcnt = 0;
static unsigned char maxcnt = 0;


// Read a byte of data from the CH375 device which
// is returned in the low 8 bits. If there is no
// data, abort the simulation.
unsigned char read_ch375_data(void) {

  unsigned char data;

  // Return the status if the previous command is GET_STATUS
  if (prevcmd == GET_STATUS)
    return (status);

  // Return the size of the buffer if the previous command is RD_USB_DATA.
  // Return a maximum of 64 bytes available.
  if (prevcmd == RD_USB_DATA) {
    prevcmd = 0;
    return ((bufcnt > 64) ? 64 : (char) bufcnt);
  }

  // No data, so abort
  if (bufcnt <= 0) {
    fprintf(stderr, "No CH375 data available to read after cmd 0x%x\n",
	    prevcmd);
    exit(1);
  }

  // Otherwise get the data, update the index and count, and return the data.
  // Once there is no data, go back to awaiting a command.
  data = buf[bufindex++];
  bufcnt--;

  return (data);
}

// Count of consecutive DISK_RD_GO or DISK_WR_GO commands
int gocount=0;

// Receive a command from the 6809. If the function returns true,
// then the 6809 must generate an interrupt.
unsigned recv_ch375_cmd(unsigned char cmd) {

  struct stat S;
  off_t numblocks;
  int err;

  switch (prevcmd = cmd) {
  case GET_IC_VER:
    bufindex = 0; bufcnt = 1; buf[0] = 0xB7; status = 0; break;
  case RESET_ALL:
    bufindex = 0; bufcnt = 0; status = 0; break; break;
  case CHECK_EXIST:
    bufindex = 0; bufcnt = 0; status = 0; break;
  case SET_USB_MODE:
    bufindex = 0; bufcnt = 0; status = 0; break;
  case DISK_INIT:
    // Try to open the filesystem image. Send an interrupt if successful
    if ((disk = fopen(ch375file, "r+")) == NULL) {
      fprintf(stderr, "Unable to open CH375 file '%s' read-write\n", ch375file);
      exit(1);
    }
    status = USB_INT_SUCCESS; return (1);
  case GET_STATUS:
    // Nothing to do here
    break;
  case DISK_SIZE:
    // Get the file's size
    if (stat(ch375file, &S) == -1) {
      fprintf(stderr, "CH375 unable to stat '%s'\n", ch375file); exit(1);
    }

    // Convert to number of blocks
    numblocks = S.st_size / 512;

    // Put the 32-bit size into the buffer big-endian
    buf[0] = (numblocks >> 24) & 0xff;
    buf[1] = (numblocks >> 16) & 0xff;
    buf[2] = (numblocks >> 8) & 0xff;
    buf[3] = numblocks & 0xff;

    // Blocks are 512 bytes in size
    buf[4] = 0x00; buf[5] = 0x00; buf[6] = 0x02; buf[7] = 0x00;

    // Send an interrupt
    bufindex = 0; bufcnt = 8; status = USB_INT_SUCCESS; return (1);
  case DISK_READ:
    // Nothing to do here
    gocount=0; bufindex=0; break;
  case RD_USB_DATA:
    // Nothing to do here
    break;
  case DISK_WRITE:
    // Nothing to do here
    gocount= 0; bufindex=0; break;
  case WR_USB_DATA:
    // Nothing to do here
    break;
  case DISK_RD_GO:
    // Read another 64 bytes into the buffer and send an interrupt
    // After eight DISK_RD_GO commands, set status to USB_INT_SUCCESS
    if ((err = fread(buf, 64, 1, disk)) != 1) {
      fprintf(stderr, "CH375 read error\n"); exit(1);
    }
    bufindex = 0; bufcnt = 64; gocount++;
    if (gocount==8)
      status = USB_INT_SUCCESS;
    else
      status = USB_INT_DISK_READ;
    return (1);
  case DISK_WR_GO:
    // Send an interrupt
    gocount++;
    if (gocount==8)
      status = USB_INT_SUCCESS;
    else
      status = USB_INT_DISK_WRITE;
    return (1);

  default:
    fprintf(stderr, "Unknown CH375 command 0x%x\n", cmd); exit(1);
  }
  return (0);
}

// Receive data from the 6809. If the
// function returns true, then the 6809
// must generate an interrupt.
unsigned recv_ch375_data(unsigned char data) {
  off_t offset;
  int err;

  switch (prevcmd) {
  case CHECK_EXIST:
    // Invert the data and put it in the buf
    bufindex = 0; bufcnt = 1; buf[0] = ~data; break;
  case SET_USB_MODE:
    // Ensure it's a 6
    if (data != 6) {
      fprintf(stderr, "Didn't get 6 after USB_MODE: 0x%x\n", data); exit(1);
    }
    // Put the status into the buffer and also send an interrupt
    status = USB_INT_CONNECT; bufindex = 0; bufcnt = 1; buf[0] = status; return(1);
  case DISK_READ:
    // Put the data into the buffer unless there's too much
    if (bufindex > 5) {
      fprintf(stderr, "Too many arguments after a DISK_READ cmd\n"); exit(1);
    }
    buf[bufindex++] = data;

    // Read the block if we have the right number of arguments.
    // Send an interrupt as a result.
    if (bufindex == 5) {
      // We can only read one block at a time
      if (buf[4] != 1) {
	fprintf(stderr, "CH375 can only read 1 block\n"); exit(1);
      }
      offset = 512 * (buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24));
      if ((err = fseek(disk, offset, SEEK_SET)) == -1) {
	fprintf(stderr, "CH375 seek error offset %ld\n", offset); exit(1);
      }

      if ((err = fread(buf, 64, 1, disk)) != 1) {
	fprintf(stderr, "CH375 read error offset %ld\n", offset); exit(1);
      }
      bufindex = 0;
      bufindex = 0; bufcnt = 64; status = USB_INT_DISK_READ; return (1);
    }
    break;

  case DISK_WRITE:
    // Put the data into the buffer unless there's too much
    if (bufindex > 5) {
      fprintf(stderr, "Too many arguments after a DISK_WRITE cmd\n"); exit(1);
    }
    buf[bufindex++] = data;

    // Seek to the specified location.
    // Send an interrupt as a result.
    if (bufindex == 5) {
      // We can only write one block at a time
      if (buf[4] != 1) {
	fprintf(stderr, "CH375 can only write 1 block\n"); exit(1);
      }
      offset = 512 * (buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24));
// printf("ch375 write to block %ld\n", offset/512);
      if ((err = fseek(disk, offset, SEEK_SET)) == -1) {
	fprintf(stderr, "CH375 seek error offset %ld\n", offset); exit(1);
      }

      bufindex = 0; status = USB_INT_DISK_WRITE; return (1);
    }
    break;

  case WR_USB_DATA:
    // Check that the maxcnt size is not too big
    if (data>64) {
	fprintf(stderr, "CH375 cannot write more than 64 bytes\n"); exit(1);
    }
    // Prepare to read maxcnt data into the buffer
    maxcnt= data; bufindex = 0; prevcmd= WR_USB_DATA1;
    break;

  case WR_USB_DATA1:
    // Check that we haven't exceeded the maximum data from the 6809
    if (bufindex == maxcnt) {
	fprintf(stderr, "More data than maxcnt in CH375\n"); exit(1);
    }
    // Store the data in the buffer
    buf[bufindex++] = data;
    // Write the data to the file if we've hit maxcnt
    if (bufindex == maxcnt) {
      if ((err = fwrite(buf, 64, 1, disk)) != 1) {
	fprintf(stderr, "CH375 write error\n"); exit(1);
      }
      fflush(disk);
      bufindex = 0; maxcnt = 0;
    }
    break;

  default:
    fprintf(stderr, "Received unwanted CH375 data after cmd %d\n", prevcmd); exit(1);
  }
  return (0);
}
