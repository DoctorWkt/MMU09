; Startup code for C code
; (c) 2022, Warren Toomey, GPL3

	section code

_printint
        export _printint
        lda #1
        swi2
	rts

_myexit
	export _myexit
;	sync
        lda #0
        swi2
	rts

