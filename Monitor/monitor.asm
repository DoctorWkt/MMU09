; This is a simple monitor for the MMU09 SBC.
; For usage details, search for "welcomemsg"
; in this file!
;
; (c) 2023 Warren Toomey, GPL3.

; Non-ROM addresses

; UART addresses
uartrd          equ     $fe20           ; UM245R read address
uartwr          equ     $fe40           ; UM245R write address

; Addresses to read & write data and commands to the CH375 device
chdatard	equ    $fe60
chdatawr	equ    $fe80
chcmdrd		equ    $fe60
chcmdwr		equ    $fe81

externswi	equ	$7ffe		; Stores address of SWI handler in RAM
					; and is preceded by a JMP instruction

stacktop	equ	$7f70		; Top of stack, set by this monitor

; CH375 commands
CMD_RESET_ALL           equ 0x05
CMD_SET_USB_MODE        equ 0x15
CMD_GET_STATUS          equ 0x22
CMD_RD_USB_DATA         equ 0x28
CMD_WR_USB_DATA         equ 0x2B
CMD_DISK_INIT           equ 0x51
CMD_DISK_SIZE           equ 0x53
CMD_DISK_READ           equ 0x54
CMD_DISK_RD_GO          equ 0x55
CMD_DISK_WRITE          equ 0x56
CMD_DISK_WR_GO          equ 0x57
CMD_DISK_READY          equ 0x59

; CH375 status results
USB_INT_SUCCESS         equ 0x14
USB_INT_CONNECT		equ 0x15
USB_INT_DISK_READ       equ 0x1D
USB_INT_DISK_WRITE      equ 0x1E

; Variables
		org	$7f80

chstatus	fcb	#$00		; CH375 status after an FIRQ
uartflg		fcb	#$00		; Flag indicating if there's a character in uartch, initially false
uartch		fcb	#$00		; UART character available to read if uartflg==1
dumpaddr	fdb	#$ff00		; Start address for memory dumping
temp1		fcb	#$00		; Temp variable
temp2		fcb	#$00		; Temp variable

; ROM code
		org	$8000

; Get a character from the UART in A
getc		lda	uartflg		; See if there is any UART data to read
		beq	getc		; Loop until there is
		lda	#$00		; Reset the status flag
		sta	uartflg
		lda	uartch		; Get the available character
		rts			; and return

; Send the character in A to the UART
putc		sta	uartwr
		rts

; Get a character from the UART into A, echoing it as well.
; \r characters get converted to \n.
;
getputc		jsr	getc		; Get a character
		cmpa	#$0d		; If we got an \r
		bne	1f		; send a \n after the \r
		jsr	putc
		lda	#$0a
1		jsr	putc
		rts

; UART IRQ Handler

uartirq		lda	uartrd		; Get the character from the UART
		sta	uartch		; and save it in the 1-char buffer
		lda	#$01		; Set the status flag
		sta	uartflg
		rti

; CH375 FIRQ Handler

; When we get a fast IRQ, send CMD_GET_STATUS to the
; CH375 to stop the interrupt. Get the current status
; and store it in chstatus. Push/pop A to ensure it's intact.
; Also print a '@' to indicate we did the FIRQ.
ch375firq	pshs	a
		lda	#$22		; CMD_GET_STATUS
		sta     chcmdwr
        	lda     chdatard        ; Get the result back
        	sta     chstatus
		puls	a
        	rti

; SWI system call handler
swihandler	cmpx	#17		; Syscall 17 is putchar()
		bne	1f
		jsr	putc
		jmp	swiend
1		cmpx	#23		; Syscall 23 is getchar()
		bne	2f
		jsr	getc
		jmp	swiend
2		cmpx	#1		; Syscall 1 is exit()
		bne	3f
		jmp	main		; Restart monitor on exit()
3		cmpx	#69		; Syscall 69 is sbrk()
		bne	4f
		jmp	swiend		; Do nothing for sbrk()
4		cmpx	#18		; Syscall 18 is printint() but in hex
		bne	5f
		jsr	prhex		; Print A then B
		tfr	b,a
		jsr	prhex
		jmp	swiend
5		jmp	externswi-1
swiend		rti

; Print out the NUL-terminated string which X points at
;
puts		pshs	a		; Save A
1		lda	,x+		; Get next char
		beq	2f		; End when we hit NUL
		sta	uartwr		; Send to the UART
		bra	1b		; and loop back
2		puls	a		; Get A back and return
		rts

; Print out the A register as two hex digits. Borrowed from the ASSIST09 code
prhex		pshs	d		; Save - do not reread
		LDB	#16		; Shift by 4 bits
		MUL			; with multiply
		jsr	prnibble	; Print the upper nibble
		puls	d		; Restore bytes
		anda	#$0f		; Isolate the lower nibble

prnibble	adda	#$90		; Prepare A-F adjust
		daa			; Adjust
		adca	#$40		; Prepare character bits
		daa			; Adjust
		jsr	putc
		rts

; Print out the word that X points to as four hex digits
prhexword	lda	,x+		; Print first byte
		jsr	prhex
		lda	,x		; and second byte
		jsr	prhex
		rts

; cnvhex: Convert hex characters in A and B to a binary byte. From ASSIST09.
; B holds the top nibble, A holds the bottom nibble. The 8-bit result is
; returned in A. B is destroyed. If a valid hex digit, Z is set to 1, else 0.
cnvhex		cmpb	#'a		; Lowercase letter?
		blo	1f		; No, skip
		andb	#$df		; Convert to uppercase
1		cmpb	#'0		; Is B below ASCII '0'?
		blo	cnvrts		; Yes, error
		cmpb	#'9		; Could it be 'A' to 'F'?
		ble	cnvtopgot	; No, it's in the '0' to '9' range
		cmpb	#'A		; Outside the 'A' to 'F' range?
		blo	cnvrts		; Yes, error
		cmpb	#'F
		bhi	cnvrts		; Yes, error
		subb	#7		; Change letter into number
cnvtopgot	andb	#$0f		; Lose unwanted bits
		lslb			; Now move to the top nibble
		lslb
		lslb
		lslb			; Time to deal with the A register
		stb	temp1
		cmpa	#'a
		blo	2f
		anda	#$df
2		cmpa	#'0		; Lower than a zero?
		blo	cnvrts		; Branch not value
		cmpa	#'9		; Possible A-F?
		ble	cnvgot		; Branch no to accept
		cmpa	#'A		; Less then ten?
		blo	cnvrts		; Return if minus (invalid)
		cmpa	#'F		; Not too large?
		bhi	cnvrts		; No, return too large
		suba	#7		; Down to binary
cnvgot		anda	#$0f		; Clear high hex
cnvok		orcc	#4		; Force zero on for valid hex
		ora	temp1		; Merge the two nibbles together
cnvrts		rts			; Return to caller

; Get the dump address from the user.
; A returns as #$00 if address set, or #$0a if not set
getdumpaddr
		jsr	getputc		; Get first nibble into B
		cmpa	#$0a		; If a newline, skip the address
		beq	2f
		tfr	a,b
		jsr	getputc		; Get second nibble into A
		jsr	cnvhex		; Convert into binary
		sta	dumpaddr
		jsr	getputc		; Get first nibble into B
		tfr	a,b
		jsr	getputc		; Get second nibble into A
		jsr	cnvhex		; Convert into binary
		sta	dumpaddr+1
		lda	#$00
2		rts

; Initialise the CH375 device. Return D=1 if OK, D=0 on error.
ch375init
	lda	#CMD_RESET_ALL		; Do a reset
	sta     chcmdwr

	ldb	#$ff			; and delay loop for reset to work
1	sta	$FFFF
	sta	$FFFF
	sta	$FFFF
	sta	$FFFF
	sta	$FFFF
	sta	$FFFF
	sta	$FFFF
	sta	$FFFF
	decb
	bne	1b

	lda	#$ff
	sta	chstatus		; Store dummy value in chstatus
	lda	#CMD_SET_USB_MODE	; Set the USB mode to 6
	ldb	#$06
	sta     chcmdwr
	stb	chdatawr

1	lda	chstatus		; Loop until we get USB_INT_CONNECT
	cmpa	#USB_INT_CONNECT
	bne	1b

	lda	#$ff
	sta	chstatus		; Store dummy value in chstatus
	lda	#CMD_DISK_INIT		; Try to init the disk
	sta     chcmdwr

2	lda	chstatus		; Loop until no dummy value
	cmpa	#$ff
	beq	2b
	cmpa	#USB_INT_SUCCESS	; Did we get USB_INT_SUCCESS?
	bne	3f			; No, error
	ldd	#1
	rts
3	ldd	#0
	rts

; Get data from the CH375 into the D register
ch375getdata
		ldb	chdatard
		lda	#$00
		rts

; Given a 4-byte LBA in big-endian format on the stack followed
; by a 2-byte buffer pointer, read the 512-byte block at that LBA
; into the given buffer. Returns D=1 if success, D=0 if failure.
readblock
	lda     #$ff
        sta     chstatus                ; Store dummy value in chstatus
	ldx	6,S			; Get the buffer's start address
	lda	#CMD_DISK_READ
	sta     chcmdwr
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

1	lda     chstatus                ; Get a real status after an interrupt
        cmpa    #$ff
        beq     1b
	cmpa	#USB_INT_DISK_READ	; Break loop if not USB_INT_DISK_READ
	bne	3f

	lda	#CMD_RD_USB_DATA	; Now read the data
	sta	chcmdwr
	ldb	chdatard		; Get the buffer size
2	lda	chdatard		; Read a data byte from the CH375
	sta	,X+			; Store the byte in the buffer
	decb
        bne     2b			; Loop until the 64 bytes are read

	lda     #$ff
        sta     chstatus                ; Store dummy value in chstatus
	lda	#CMD_DISK_RD_GO		; Tell the CH375 to repeat
	sta	chcmdwr
	jmp	1b

3	cmpa	#USB_INT_SUCCESS	; Set D=1 if we have USB_INT_SUCCESS
	beq	4f
	ldd	#0			; Otherwise set D=0.
	rts
4	ldd	#1
	rts

; Given a 4-byte LBA in big-endian format on the stack followed
; by a 2-byte buffer pointer, write the 512-byte block at that LBA
; from the given buffer. Returns D=1 if success, D=0 if failure.
writeblock
	lda     #$ff
        sta     chstatus                ; Store dummy value in chstatus

	ldx	6,S			; Get the buffer's start address
	lda	#CMD_DISK_WRITE
	sta     chcmdwr
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

1	lda     chstatus                ; Get a real status after an interrupt
        cmpa    #$ff
        beq     1b
	cmpa	#USB_INT_DISK_WRITE	; Break loop if not USB_INT_DISK_WRITE
	bne	3f

	lda	#CMD_WR_USB_DATA	; Now write the data
	sta	chcmdwr
	ldb	#$40			; 64 bytes at a time
	stb	chdatawr		; Send the buffer size
2	lda	,X+			; Read the byte from the buffer
	sta	chdatawr		; and send to the CH375
	decb
        bne     2b			; Loop until all 64 bytes are sent

	lda     #$ff
        sta     chstatus                ; Store dummy value in chstatus
	lda	#CMD_DISK_WR_GO		; Tell the CH375 to repeat
	sta	chcmdwr
	jmp	1b

3	cmpa	#USB_INT_SUCCESS	; Set D=1 if we have USB_INT_SUCCESS
	beq	4f
	ldd	#0			; Otherwise set D=0.
	rts
4	ldd	#1
	rts

; Print out the usage information
usage		ldx	#usagemsg
		jsr	puts
		rts

; Message strings

welcomemsg	fcn	"Warren's Simple 6809 Monitor, $Revision: 1.1 $\r\n\r\n"

usagemsg	fcc	"DXXXX - dump 16 bytes at $XXXX. If D by itself,\r\n"
		fcc	"        dump starting past the last dump command\r\n"
		fcc	"AXXXX XX XX XX XX ...\r\n"
		fcc	"      - alter memory at $XXXX with new bytes\r\n"
		fcc	"GXXXX - run code at $XXXX\r\n"
		fcc	"L     - load s19 file into memory\r\n"
		fcc	"?     - show this usage message\r\n"
		fcn	"XX stands for hex digits: 0-9, a-f, A-F\r\n"

unknownmsg	fcn	"Unknown command\r\n\r\n"

promptmsg	fcn	"> "
colonmsg	fcn	": "
crlfmsg		fcn	"\r\n"

s19msg		fcc	"Type in an s19 file with only S1 and S9 lines.\r\n"
		fcn	"Ensure the last line is the S9 line.\r\n"

s19errmsg	fcn	"Unrecognised S line\r\n"


; The main monitor code
		org	$f800

main		lds	#stacktop	; Set up the stack pointer
		andcc	#$af		; Enable interrupts
		lda	#$00		; Reset the status flag
		sta	uartflg
		lda	#$7E		; Put a 'jmp' instruction
		sta	externswi-1	; before the address of the
					; external SWI handler

		ldx	#welcomemsg	; Print out the welcome message and the usage message
		jsr	puts
uprompt		jsr	usage

prompt		ldx	#promptmsg	; Print out the prompt
		jsr	puts

getcmd		jsr	getputc		; Get the command letter
		cmpa	#'d
		beq	dump		; d or D is dump
		cmpa	#'D
		beq	dump
		cmpa	#'l
		lbeq	load		; l or L is load
		cmpa	#'L
		lbeq	load
		cmpa	#'g
		beq	go		; g or G is execute code
		cmpa	#'G
		beq	go
		cmpa	#'a
		lbeq	alter
		cmpa	#'A
		lbeq	alter

unknown		jsr	getputc		; Unknown command. Loop getting
		cmpa	#$0a		; characters until EOLN
		bne	unknown
unknown2	ldx	#unknownmsg	; Print out error message, show
		jsr	puts		; usage and reprompt
		jmp	uprompt

dump		
		jsr	getdumpaddr	; Get the dump address
		cmpa	#$0a		; Skip EOLN loop if we are there
		beq	2f		; already
1		jsr	getputc		; Loop getting
		cmpa	#$0a		; characters until EOLN
		bne	1b
2		ldx	#dumpaddr	; Print out the dump address
		jsr	prhexword
		ldx	#colonmsg	; and the colon
		jsr	puts
		ldx	dumpaddr
		ldb	#$00		; Start with B at zero
3		lda	,x+		; Get a byte to dump
		jsr	prhex		; Print it in hex
		lda	#$20		; followed by a space
		jsr	putc
		incb
		cmpb	#$10		; Loop until B is 16
		bne	3b

		ldd	dumpaddr	; Increment the dump address
		addd	#$10		; by 16
		std	dumpaddr

		ldx	#crlfmsg	; Move to the next line
		jsr	puts
		jmp	prompt		; and prompt for next command
		
go
		jsr	getdumpaddr	; Get the dump address
		cmpa	#$0a		; Skip EOLN loop if we are there
		beq	2f		; already
1		jsr	getputc		; Loop getting
		cmpa	#$0a		; characters until EOLN
		bne	1b
2		jsr	[dumpaddr]	; Call subroutine at the dump address

		ldx	#crlfmsg	; Move to the next line
		jsr	puts
		jmp	prompt		; and prompt for next command

load
1		jsr	getputc		; Loop getting
		cmpa	#$0a		; characters until EOLN
		bne	1b
		ldx	#s19msg		; Prompt for the upload
		jsr	puts

gets19line
		jsr	getputc		; Get the initial 'S' character
		cmpa	#'S
		bne	s19err		; Error if not an 'S'

		jsr	getputc		; Get a '1' or '9'
		cmpa	#'1
		beq	s19line1
		cmpa	#'9
		beq	s19line9
s19err
		jsr	getputc		; Loop getting
		cmpa	#$0a		; characters until EOLN
		bne	s19err
		ldx	#s19errmsg	; Print out the error message
		jsr	puts
		jmp	prompt		; and prompt for next command

s19line1
		jsr	getputc		; Get the line length
		tfr	a,b
		jsr	getputc		; Get second nibble into A
		jsr	cnvhex		; Convert into binary
		sta	temp2
		jsr	getdumpaddr	; Get the address of this line

1		jsr	getputc		; Get a nibble or EOLN
		cmpa	#$0a
		beq	gets19line
		tfr	a,b
		jsr	getputc		; Get second nibble into A
		jsr	cnvhex		; Convert into binary
		ldx	dumpaddr
		sta	,x+		; Save value into memory
		stx	dumpaddr	; and update the address
		jmp	1b

s19line9
1		jsr	getputc		; Loop getting
		cmpa	#$0a		; characters until EOLN
		bne	1b

		jmp	prompt		; and prompt for next command

alter		jsr	getdumpaddr	; Get the alter address
		cmpa	#$0a		; Error if we get a newline
		lbeq	unknown2
		jsr	getputc		; Get the space after the address
		cmpa	#$0a		; Error if we get a newline
		lbeq	unknown2
alterbytes				; Now get a byte
		jsr	getputc		; Get first nibble into B
		cmpa	#$0a		; Error if we get a newline
		lbeq	unknown2
		tfr	a,b
		jsr	getputc		; Get second nibble into A
		cmpa	#$0a		; Error if we get a newline
		lbeq	unknown2
		jsr	cnvhex		; Convert into binary
		ldx	dumpaddr	; Put the byte at the dump address
		sta	,x+
		stx	dumpaddr	; and update the address
		jsr	getputc		; Get the character after the byte
		cmpa	#$0a		; If newline, end of alter
		lbeq	prompt
		jmp	alterbytes

; Vector table for the interrupt handlers and boot code

		org	$fff6
		fdb	ch375firq
		fdb	uartirq
		fdb	swihandler
		org	$fffe
		fdb	main
		end
