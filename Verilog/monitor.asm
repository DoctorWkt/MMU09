; This is code to test the MMU in the design. It will change frequently!!
; Don't expect anything in here to be useful :-)


; Memory locations for the I/O devices and the page table entries
;
uart		equ $fe00
pte0		equ $fec0
pte1		equ $fec1
pte2		equ $fec2
pte3		equ $fec3
pte4		equ $fec4
pte5		equ $fec5
pte6		equ $fec6
pte7		equ $fec7
umodeswitch	equ $fef0		; Leave kernel mode on a write here
cpuhalt		equ $fff0		; Terminates iverilog simulation

linefeed	equ $0a

		org $8000		; Start the ROM at $8000
		nop

		org $ff00		; ROM code which is always available
main		
		lda #$80		; Set up a valid frame for page 0
		sta pte0
		lda #$87		; Set up a valid frame for page 7
		sta pte7		; (to cover ROM when we go to umode)
		lda #$01		; Set up an invalid frame for page 1
		sta pte1
		lda #$82		; Set up an valid frame for page 2
		sta pte2
		lds #$5000		; Set the stack well away from page 1

		lda #'w'		; Check RAM read/write on page 0
		sta $1234
		ldb $1234

		stb uart		; Send some output
		lda #'k'
		sta uart
		lda #'t'
		sta uart
		lda #linefeed
		sta uart

		sta umodeswitch		; Now enable user mode
		nop
		nop

		; pshs a		; Push stuff on the stack. This
		; pshs a		; should cause a page fault.
		; pshs a
		

		swi3			; Call the kernel. This should set BS
		nop			; and cause us to go into kernel mode.
		nop
		nop
		nop
		sta cpuhalt		; Halt the CPU

swi
nmi
		nop
		nop
		lda #$81		; Make page 1 valid
		sta pte1
		sta umodeswitch		; and return to user mode
		rti
		
		org $fff2		; Interrupt vectors
		fdb swi			; SWI3
		fdb swi			; SWI2
		fdb main		; FIRQ
		fdb main		; IRQ
		fdb swi			; SWI
		fdb nmi			; NMI
		fdb main		; Reset
		end
