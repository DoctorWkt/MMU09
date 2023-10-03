; Non-ROM addresses

; UART addresses
uartrd          equ     $fe10           ; UM245R read address
uartwr          equ     $fe20           ; UM245R write address

; Addresses to read & write data and commands to the CH375 device
chdatard	equ    $fe30
chdatawr	equ    $fe40
chcmdrd		equ    $fe31
chcmdwr		equ    $fe41

; Addresses to enable/disable the 32K ROM
; and to disable the I/O area at $FExx
disablerom	equ	$fe50
enablerom	equ	$fe51
disableiorom	equ	$fe60		; Disable both I/O and ROM
prevmode	equ	$fe80		; Go back to previous user/kernel mode

; Page table entries
pte0		equ	$fe70
pte1		equ	$fe71
pte2		equ	$fe72
pte3		equ	$fe73
pte4		equ	$fe74
pte5		equ	$fe75
pte6		equ	$fe76
pte7		equ	$fe77

stacktop	equ	$1fff

; Variables
		org	$7780

uartflg		fcb	#$00		; Flag indicating if there's a character in uartch, initially false
uartch		fcb	#$00		; UART character available to read if uartflg==1

; ROM starts at $8000 as it is a 32K part
		org	$8000
		nop

; ROM code
		org	$e000

; User code which gets copied down to $0000

usercode	lda	#'E'		; Go to kernel mode, print H
		ldx	#17
		swi2
		lda	#'c'
		ldx	#17
		swi2
		lda	#'h'
		ldx	#17
		swi2
		lda	#'o'
		ldx	#17
		swi2
		lda	#':'
		ldx	#17
		swi2
		lda	#' '
		ldx	#17
		swi2
1		ldx	#23		; Get a character from the UART
		swi2
		ldx	#17		; and echo it back
		swi2
		bra	1b

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

; Top 256 bytes of ROM. This part of ROM can never get mapped out.
		org	$ff00

; UART IRQ Handler

uartirq         lda     uartrd          ; Get the character from the UART
                sta     uartch          ; and save it in the 1-char buffer
                lda     #$01            ; Set the status flag
                sta     uartflg
		sta     prevmode	; Go back to the previous mode
                rti

; SWI2 system call handler
swi2handler	cmpx	#17		; Syscall 17 is putchar()
		bne	1f
		jsr	putc
		jmp	swiend
1               cmpx    #23             ; Syscall 23 is getchar()
                bne     2f
                jsr     getc
		sta	1,S		; Overwrite the A on the RTI stack
                jmp     swiend
2		cmpx	#1		; Syscall 1 is exit()
		bne	3f
		jmp	main		; Restart monitor on exit()
3
swiend		sta	prevmode	; Go back to the previous mode
		rti


main
                lda     #$00            ; Set up 32K of RAM
                sta     pte0
		lda	#$01
                sta     pte1
		lda	#$02
                sta     pte2
		lda	#$03
                sta     pte3
		sta	uartflg		; Mark no available UART input
		lds     #stacktop       ; Set up the stack pointer

		ldx	#$0000		; Copy stuff from $E000 to $0000
		ldy	#$e000
1
		ldd	,y++
		std	,x++
		cmpx	#$0100
		bne	1b

		sta	disableiorom	; Disable the 32K ROM and the I/O area
                andcc   #$af            ; Enable interrupts
		jmp	$0000		; Jump to the user code

; Vector table for the interrupt handlers and boot code

		org	$fff4
		fdb	swi2handler
		org	$fff8
		fdb	uartirq
		org	$fffe
		fdb	main
		end
