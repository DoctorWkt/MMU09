; Read block 1 from the CH375.
; Run in kernel mode.

; Initialise the CH375 device. Return D=1 if OK, D=0 on error.
ch375init equ	$e0b5

; Given a 4-byte LBA in big-endian format on the stack followed
; by a 2-byte buffer pointer, read the 512-byte block at that LBA
; into the given buffer. Returns D=1 if success, D=0 if failure.
readblock equ 	$e110

; Send the character in A to the UART.
putc 	equ 	$e00e

; Print out the A register as two hex digits.
prhex	equ	$e030

	org	$0002

; Initialise the CH375
	jsr	ch375init

; Move B to A so we can print out the result
	tfr	b,a
	jsr	prhex
	lda	#$0A
	jsr	putc

; Push block 1 and buffer $1000
; and read the block
	ldd	#$1000
	pshs	d
	ldd	#$0001
	pshs	d
	ldd	#$0000
	pshs	d
	jsr	readblock
	leas	6,S

; Move B to A so we can print out the result
	tfr	b,a
	jsr	prhex
	lda	#$0A
	jsr	putc

	rts
