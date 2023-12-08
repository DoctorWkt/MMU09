; Non-ROM addresses

; UART addresses
uartrd          equ     $fe10           ; UM245R read address
uartwr          equ     $fe20           ; UM245R write address

; Addresses to read & write data and commands to the CH375 device
chdatard	equ    $fe30
chdatawr	equ    $fe40
chcmdrd		equ    $fe31
chcmdwr		equ    $fe41

; Addresses to enable/disable the 24K ROM
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
		org	$0000

uartflg		fcb	#$00		; Flag indicating if there's a character in uartch, initially false
uartch		fcb	#$00		; UART character available to read if uartflg==1

; ROM starts at $2000 even though it is a 32K part
		org	$2000

main
		lda	#$00		; Put 8K of RAM at $0000
		sta	pte0
                lda     #$04            ; and 32K of RAM from $8000 up
                sta     pte4
		lda	#$05
                sta     pte5
		lda	#$06
                sta     pte6
		lda	#$07
                sta     pte7
		lda	#'W'		; Print out a character
		sta	uartwr

		lda	#'a'		; Check we can write to RAM
		sta	$0000
		lda	#'r'
		sta	$8000
		sta	$9000
		lda	#'e'
		sta	$C000
		lda	#'n'
		sta	$D000
		lda	#$0A
		sta	$E000

		lda	$0000		; and read back from it
		sta	uartwr
		lda	$8000
		sta	uartwr
		lda	$9000
		sta	uartwr
		lda	$C000
		sta	uartwr
		lda	$D000
		sta	uartwr
		lda	$E000
		sta	uartwr

		lda	$ff00		; Try to stop the simulation
loop		jmp	loop


; Vector table for the interrupt handlers and boot code

		org	$fff4
		fdb	main
		org	$fff8
		fdb	main
		org	$fffe
		fdb	main
		end
