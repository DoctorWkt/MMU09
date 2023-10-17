; Nine-E C startup code. Modified from the original Vbcc startup code.

	.text
	pshs	D		; Save argc
	clrb
	ldy	#__BS
	ldx	#__BL
	beq	l1
bssloop:			; Clear the bss region
	stb	,y+
	leax	-1,x
	bne	bssloop
l1:
	ldy	#__DS
	ldu	#__DC
	ldx	#__DL
	beq	l2
dataloop:			; Initialise the rwdata region
	ldb	,u+
	stb	,y+
	leax	-1,x
	bne	dataloop
l2:
	jsr	 __stdio_init_vars	; Set up standard I/O
	puls	D			; Get argc back
	jsr	main			; Call the main() function
	jmp	exit			; Call exit()
