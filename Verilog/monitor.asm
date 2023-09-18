; This is code to test the MMU in the design. It will change frequently!!
; Don't expect anything in here to be useful :-)


; Memory locations for the I/O devices and the page table entries
;
uartwr		equ $fe20
disablerom	equ $fe51		; Map out the 32K of ROM
enablerom	equ $fe51		; Map in the 32K of ROM
disableio	equ $fe60		; Disable the I/O area
pte0		equ $fe70
pte1		equ $fe71
pte2		equ $fe72
pte3		equ $fe73
pte4		equ $fe74
pte5		equ $fe75
pte6		equ $fe76
pte7		equ $fe77
cpuhalt		equ $fff0		; Terminates iverilog simulation

linefeed	equ $0a

		org $8000		; Start the ROM at $8000
		nop

		org $ff80		; ROM code which is always available
main		
		sta enablerom		; Turn on the 32K of ROM
		lda #$00		; Set up a valid frame for page 0
		sta pte0
		lda #$04		; Set up a valid frame for page 4
		sta pte4		; (to cover ROM when we go to umode)
		lds #$1000		; Put the stack on page 0

					; Turn off the I/O area, then do
					; an SWI to turn it back on
		sta disableio
		swi

		lda #'w'		; Send some UART output
		sta uartwr
		lda #'k'
		sta uartwr
		lda #'t'
		sta uartwr
		lda #linefeed
		sta uartwr

		sta cpuhalt		; Halt the CPU
swi
		rti
		
		org $fff2		; Interrupt vectors
		fdb swi			; SWI3
		fdb swi			; SWI2
		fdb swi			; FIRQ
		fdb swi			; IRQ
		fdb swi			; SWI
		fdb swi			; NMI
		fdb main		; Reset
		end
