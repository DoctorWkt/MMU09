	section data
foo	fdb  700		; int foo=17

	section text
.r0	extern
.r1	extern
printint extern

main
	export main

	ldd #19			; Store 19 into two registers
	std <.r0
	std <.r1
	pshs d			; Print 19 out as an integer
	jsr printint
	puls d
	ldd foo
	pshs d			; Print foo out as an integer
	jsr printint
	puls d
	lda #0
	swi2
	rts
