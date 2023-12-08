; Assembly code to initialise the hardware and do
; basic I/O on the UART and the CH375 block device.
; Also, a jump table for the XV6 filesystem calls
; and the vector table for IRQs and reset.

; Page table addresses
	.set pte0, 0xfe70
	.set pte1, 0xfe71
	.set pte2, 0xfe72
	.set pte3, 0xfe73
	.set pte4, 0xfe74
	.set pte5, 0xfe75
	.set pte6, 0xfe76
	.set pte7, 0xfe77

; Enable/disable 24K ROM.
	.set disablerom, 0xfe50
	.set enablerom,	 0xfe51

; Map out the I/O area and 24K ROM.
	.set disableio,	 0xfe60

; Go back to previous user/kernel mode.
	.set prevmode,	0xfe80

; UART read & write addresses
	.set uartrd,	0xfe10
	.set uartwr,	0xfe20

; Addresses to read & write data and commands to the CH375 device
	.set chdatard,	0xfe30
	.set chdatawr,	0xfe40
	.set chcmdrd,	0xfe31
	.set chcmdwr,	0xfe41

; CH375 commands
	.set CMD_RESET_ALL,    0x05
	.set CMD_SET_USB_MODE, 0x15
	.set CMD_GET_STATUS,   0x22
	.set CMD_RD_USB_DATA,  0x28
	.set CMD_WR_USB_DATA,  0x2B
	.set CMD_DISK_INIT,    0x51
	.set CMD_DISK_SIZE,    0x53
	.set CMD_DISK_READ,    0x54
	.set CMD_DISK_RD_GO,   0x55
	.set CMD_DISK_WRITE,   0x56
	.set CMD_DISK_WR_GO,   0x57
	.set CMD_DISK_READY,   0x59

; CH375 status results
	.set USB_INT_SUCCESS,	 0x14
	.set USB_INT_CONNECT,	 0x15
	.set USB_INT_DISK_READ,	 0x1D
	.set USB_INT_DISK_WRITE, 0x1E

; Terminal characteristics
	.set TC_ECHO,		0x01	; Echo input characters

; Start of user code.
	.set usercode,	0x0002

; When booting, the kernel stack is in the
; kernel data page. Once we have a process
; context, we will use the user's stack.
	.set kernstack, 0x1FFF

; Uninitialised variables
	.bss
chstatus:	.zero 1			; CH375 status after an FIRQ
uartflg:	.zero 1			; Flag indicating if char in uartch,
					; initially zero (false)
uartch:		.zero 1			; UART character available to read
					; if uartflg==1
frame0:		.zero 1			; Which frame is at page zero. Could be
		.global frame0		; a user or a kernel frame
uframe0:	.zero 1			; Which user frame last at page zero
		.global uframe0
termattr:	.zero 2			; Terminal characteristics

usersp:		.zero 2			; Holds old user stack pointer
userd:		.zero 2			; Holds D register when handling SWI2
userx:		.zero 2			; Holds X register when handling SWI2

; The jump table of vectors to the XV6 filesystem calls in the ROM.
	.section .jumptable, "acrx"
syscalltable:
	.global syscalltable
	.word sys_exit			; Offset $00
	.word romgetputc		; Offset $02
	.word romputc			; Offset $04
	.word sys_exec			; Offset $06
	.word sys_chdir			; Offset $08
	.word sys_close			; Offset $0A
	.word sys_dup			; Offset $0C
	.word sys_fstat			; Offset $0E
	.word sys_link			; Offset $10
	.word sys_lseek			; Offset $12
	.word sys_mkdir			; Offset $14
	.word sys_open			; Offset $16
	.word sys_read			; Offset $18
	.word sys_unlink		; Offset $1A
	.word sys_write			; Offset $1C
	.word tcattr			; Offset $1E
	.word sys_fork			; Offset $20
	.word sys_wait			; Offset $22
	.word sys_getpid		; Offset $24
	.word sys_kill			; Offset $26
	.word sys_pipe			; Offset $28

; ROM routines
	.text

; These two functions allow us to save the stack pointer before
; we enter the real functions. That way, we can return through
; sched() regardless of what locals & pushed arguments happen
; in the real functions.

sys_fork:
	.global sys_fork
	sts	_schedsp
	jmp	fork1

sched:
	.global	sched
	sts	_schedsp
	jmp	sched1

; Get and/or set the current terminal characteristics. D holds the
; command: 1 means set. The new value is on the stack. Always return
; the current value.
tcattr:
	.global tcattr
	cmpd	#0x01		; Shall we set the value?
	bne	tcattr01
	ldd	2,S		; Get the new value
	std	termattr	; and store it
tcattr01:
	ldd	termattr	; Load the current value
	rts			; and return it

; Get a character from the UART into D with the top byte zero
getc:	lda	uartflg		; See if there is any UART data to read
	beq	getc		; Loop until there is
	clra			; Reset the status flag
	sta	uartflg
	ldb	uartch		; Get the available character
	rts			; and return

; Send the character in D to the UART
romputc:
	.global	romputc
	stb	uartwr
	rts

; Print the string which is the argument to the UART, followed by a
; newline. Then loop indefinitely.
panic:
	.global	panic
	tfr	D,X		; Get the pointer
panic1: ldb	,x+		; Get next char
	beq	panic2		; End when we hit NUL
	stb	uartwr		; Send to the UART
	bra	panic1		; and loop back
panic2:	ldb	#0x0A		; Send a \n to the UART
	stb	uartwr
panic3:	jmp	panic3

; Get a character from the UART into D, echoing it as well.
; \r characters get converted to \n.
;
romgetputc:
	.global romgetputc
	ldd	termattr
	andb	#TC_ECHO	; Do we need to echo the character?
	bne	getpc1		; Yes
	jmp	getc		; No, get a character and return it
getpc1:	jsr	getc		; Get a character
	cmpb	#0x0d		; If we got an \r
	bne	L1		; send a \n after the \r
	stb	uartwr
	ldb	#0x0a
L1:	stb	uartwr
	clra
	rts

; Print out the A register as two hex digits. Borrowed from the ASSIST09 code
prhex:		pshs	d		; Save - do not reread
		LDB	#16		; Shift by 4 bits
		MUL			; with multiply
		jsr	prnibble	; Print the upper nibble
		puls	d		; Restore bytes
		anda	#0x0f		; Isolate the lower nibble

prnibble:	adda	#0x90		; Prepare A-F adjust
		daa			; Adjust
		adca	#0x40		; Prepare character bits
		daa			; Adjust
		tfr	A,B
		jsr	romputc
		rts

; Print out the word that X points to as four hex digits
prhexword:
	.global prhexword
		lda	,x+		; Print first byte
		jsr	prhex
		lda	,x+		; and second byte
		jsr	prhex
		rts

; Initialise the CH375 device. Return D=1 if OK, D=0 on error.
ch375init:
	.global ch375init
	lda	#CMD_RESET_ALL		; Do a reset
	sta	chcmdwr

	ldb	#0x00			; and delay loop for reset to work.
L4:	sta	usersp			; We use usersp as a place to write
	sta	usersp			; as it's not being used yet.
	sta	usersp
	sta	usersp
	sta	usersp
	sta	usersp
	sta	usersp
	sta	usersp
	decb
	bne	L4

	lda	#0xff
	sta	chstatus		; Store dummy value in chstatus
	lda	#CMD_SET_USB_MODE	; Set the USB mode to 6
	ldb	#0x06
	sta	chcmdwr
	stb	chdatawr

L5:	lda	chstatus		; Loop until we get USB_INT_CONNECT
	cmpa	#USB_INT_CONNECT
	bne	L5

	lda	#0xff
	sta	chstatus		; Store dummy value in chstatus
	lda	#CMD_DISK_INIT		; Try to init the disk
	sta	chcmdwr

L6:	lda	chstatus		; Loop until no dummy value
	cmpa	#0xff
	beq	L6
	cmpa	#USB_INT_SUCCESS	; Did we get USB_INT_SUCCESS?
	bne	L7			; No, error
	ldd	#1
	rts
L7:	ldd	#0
	rts

; Given a 2-byte buffer pointer in D and a 4-byte LBA in big-endian format
; on the stack, read the 512-byte block at that LBA into the given buffer.
; Returns D=1 if success, D=0 if failure.
readblock:
	.global readblock
	tfr	D,X			; Get the buffer's start address
	lda	#0xff
	sta	chstatus		; Store dummy value in chstatus
	lda	#CMD_DISK_READ
	sta	chcmdwr
	lda	5,S			; Send the LBA little-endian
	sta	chdatawr
	lda	4,S
	sta	chdatawr
	lda	3,S
	sta	chdatawr
	lda	2,S
	sta	chdatawr
	lda	#1			; and read one block
	sta	chdatawr

L8:	lda	chstatus		; Get a real status after an interrupt
	cmpa	#0xff
	beq	L8
	cmpa	#USB_INT_DISK_READ	; Break loop if not USB_INT_DISK_READ
	bne	L10

	lda	#CMD_RD_USB_DATA	; Now read the data
	sta	chcmdwr
	ldb	chdatard		; Get the buffer size
L9:	lda	chdatard		; Read a data byte from the CH375
	sta	,X+			; Store the byte in the buffer
	decb
	bne	L9			; Loop until the 64 bytes are read

	lda	#0xff
	sta	chstatus		; Store dummy value in chstatus
	lda	#CMD_DISK_RD_GO		; Tell the CH375 to repeat
	sta	chcmdwr
	jmp	L8

L10:	cmpa	#USB_INT_SUCCESS	; Set D=1 if we have USB_INT_SUCCESS
	beq	L11
	ldd	#0			; Otherwise set D=0.
	rts
L11:	ldd	#1
	rts

; Given a 2-byte buffer pointer in D and a 4-byte LBA in big-endian format
; on the stack, write the 512-byte block from the buffer to that LBA.
; Returns D=1 if success, D=0 if failure.
writeblock:
	.global writeblock
	tfr	D,X			; Get the buffer's start address
	lda	#0xff
	sta	chstatus		; Store dummy value in chstatus
	lda	#CMD_DISK_WRITE
	sta	chcmdwr
	lda	5,S			; Send the LBA little-endian
	sta	chdatawr
	lda	4,S
	sta	chdatawr
	lda	3,S
	sta	chdatawr
	lda	2,S
	sta	chdatawr
	lda	#1			; and write one block
	sta	chdatawr

L12:	lda	chstatus		; Get a real status after an interrupt
	cmpa	#0xff
	beq	L12
	cmpa	#USB_INT_DISK_WRITE	; Break loop if not USB_INT_DISK_WRITE
	bne	L14

	lda	#CMD_WR_USB_DATA	; Now write the data
	sta	chcmdwr
	ldb	#0x40			; 64 bytes at a time
	stb	chdatawr		; Send the buffer size
L13:	lda	,X+			; Read the byte from the buffer
	sta	chdatawr		; and send to the CH375
	decb
	bne	L13			; Loop until all 64 bytes are sent

	lda	#0xff
	sta	chstatus		; Store dummy value in chstatus
	lda	#CMD_DISK_WR_GO		; Tell the CH375 to repeat
	sta	chcmdwr
	jmp	L12

L14:	cmpa	#USB_INT_SUCCESS	; Set D=1 if we have USB_INT_SUCCESS
	beq	L15
	ldd	#0			; Otherwise set D=0.
	rts
L15:	ldd	#1
	rts

; The code which is performed on a reset
reset:
	.global	reset
	lda	#0		; Allocate page frame 0 for kernel data
	sta	pte0
	sta	frame0
	lds	#kernstack	; Set up the stack pointer
	andcc	#0xaf		; Enable interrupts

	clra			; Clear the kernel data page
	clrb
	tfr	D,X
.1:	std	,X++
	cmpx	#0x2000
	bne	.1

	clra			; Reset the UART status flag
	sta	uartflg
	ldd	#TC_ECHO	; Set echo mode on the terminal
	std	termattr

	jsr	sys_init	; Initialise the filesystem structures

; SWI2 handler. We were previously in user mode but now in kernel mode.
; We start with the I/O area and 24K ROM mapped in.
; D holds the first argument to the system call
; and X holds the system call number.

swi2handler:
	tfr	D,Y		; Save D temporarily
	lda	#0		; Bring back in the kernel data area
	sta	pte0
	sta	frame0
	tfr	Y,D		; Get D back
	jsr	[syscalltable,X] ; Call the relevant system call
	tfr	D,Y		; Save D temporarily
	lda	uframe0		; Replace the kernel data with user code
	sta	pte0
	tfr	Y,D		; Get D back
	ldx	errno		; and put errno in the X register.
	std	1,S		; Save any return value in the D register
	stx	4,S		; and any errno value in the X register.
	jmp	swi2end

; The 256 bytes of ROM which is always mapped in to memory
	.section .toprom, "acrx"

; void jmptouser(int memsize, int argc, char *destbuf)
;
; The user's code has been loaded into memory. We need to
; copy the arguments to the top of stack and start the code
; executing. D holds the amount of data to copy, with the
; argv and the new stack pointer value are on the stack.
jmptouser:
	.global jmptouser
	ldx	2,S		; Save the argc for now
	stx	userd
	ldx	4,S		; Save the destbuf pointer for now
	stx	userx
	pshs	X		; Set up args for rommemcpy
	ldx	#execbuf
	pshs	X
	jsr	rommemcpy	; Copy the arguments

	lds	userx		; Set the new stack pointer value
	ldd	userd		; Get the argc value
	tfr	D,Y		; Save D as we need to use it
	lda	uframe0		; Take away the kernel data on page zero and
	sta	pte0		; replace with the user code.
	tfr	Y,D		; Get D back
	sta	disableio	; Turn off the I/O area
	jmp	usercode	; Now jump to the executable's start address

; void rommemcpy(int count, void *src, void *dst)
; Copy count bytes data from src buffer to dest buffer.

rommemcpy:
	.global rommemcpy
	sta	disablerom	; Turn off the 24K of ROM
	pshs	Y		; Save the Y register. Args now 4,S and 6,S.
	ldx	4,S		; Get the source pointer
	ldy	6,S		; Get the destination pointer
	addd	6,S		; Increment the argument by the count
	std	6,S

.1:
	ldb	,X+		; Copy between the buffers
	stb	,Y+
	cmpy	6,S		; Have we reached the end of the buffer?
	bne	.1		; Not yet, loop back

	puls	Y		; Restore the Y register
	sta	enablerom	; Turn on the 24K of ROM
	rts

; void romstrncpy(int count, void *src, void *dst)
; Copy no more than count bytes of string data from
; src buffer to dest buffer. Stop when the first NUL
; byte is copied. Ensure the dest buffer is NUL terminated.
romstrncpy:
	.global romstrncpy
	sta	disablerom	; Turn off the 24K of ROM
	pshs	Y		; Save the Y register. Args now 4,S and 6,S.
	ldx	4,S		; Get the source pointer
	ldy	6,S		; Get the destination pointer
	addd	6,S		; Increment the argument by the count
	std	6,S

.1:
	ldb	,X+		; Copy between the buffers
	stb	,Y+
	beq	.2		; Break the loop if we copied a NUL
	cmpy	6,S		; Have we reached the end of the buffer?
	bne	.1		; Not yet, loop back

	clr	-1,Y		; We copied count, so put a NUL at the end
.2:
	puls	Y		; Restore the Y register
	sta	enablerom	; Turn on the 24K of ROM
	rts

; int romstrlen(char *): count the length of a string
; which could be in userspace
romstrlen:
	.global romstrlen
	sta	disablerom	; Turn off the 24K of ROM
	tfr	D,X		; Get the pointer into X
	clra			; Set the count to zero
	clrb
.1:
	tst	,X+		; Is this byte zero?
	beq	.2		; Yes, leave the loop
	addd	#1		; Else increment the count and loop back
	bra	.1
.2:
	sta	enablerom	; Turn on the 24K of ROM
	rts

; UART IRQ Handler

uartirq:
	.global uartirq
	lda	#0		; Bring in the kernel data page zero
	sta	pte0
	lda	uartrd		; Get the character from the UART
	sta	uartch		; and save it in the 1-char buffer
	lda	#0x01		; Set the status flag
	sta	uartflg
	lda	frame0		; Retore the original page zero
	sta	pte0
	sta	prevmode	; Go back to the previous user/kernel mode
	rti

; CH375 FIRQ Handler

; When we get a fast IRQ, send CMD_GET_STATUS to the
; CH375 to stop the interrupt. Get the current status
; and store it in chstatus. Push/pop A to ensure it's intact.
ch375firq:
	.global ch375firq
	pshs	a
	lda	#0		; Bring in the kernel data page zero
	sta	pte0
	lda	#CMD_GET_STATUS
	sta	chcmdwr
	lda	chdatard	; Get the result back
	sta	chstatus
	lda	frame0		; Retore the original page zero
	sta	pte0
	sta	prevmode	; Go back to the previous user/kernel mode
	puls	a
	rti

swi2end:
	sta	disableio	; Disable the I/O area
	rti			; and return from the SWI

; The vector table for SWI2, SWI, NMI, IRQ, FIRQ and reset.
	.section .vectors, "acrx"
	.word	swi2handler
	.word	ch375firq
	.word	uartirq
	.word	0x1234
	.word	0x5678
	.word	reset
