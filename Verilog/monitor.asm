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
		org	$ff10

main
		lda	#$07
		sta	pte7
                lda     #$00            ; Set up 32K of RAM
                sta     pte0
		lda	#$01
                sta     pte1
		ldd	#$1234
		std	$2000
		lda	#$08
                sta     pte1
		ldd	#$5678
		std	$2000
		lda	#$01
                sta     pte1
		ldd	$2000
		jmp	$ff00


; Vector table for the interrupt handlers and boot code

		org	$fff4
		fdb	main
		org	$fff8
		fdb	main
		org	$fffe
		fdb	main
		end
