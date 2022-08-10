; Startup code for C code
; (c) 2022, Warren Toomey, GPL3

; Set aside sixteen 4-byte locations
; on page zero to be used as registers
	section registers
.r0	zmb 4
.r1	zmb 4
.r2	zmb 4
.r3	zmb 4
.r4	zmb 4
.r5	zmb 4
.r6	zmb 4
.r7	zmb 4
.r8	zmb 4
.r9	zmb 4
.r10	zmb 4
.r11	zmb 4
.r12	zmb 4
.r13	zmb 4
.r14	zmb 4
.r15	zmb 4
	export .r0
	export .r1
	export .r2
	export .r3
	export .r4
	export .r5
	export .r6
	export .r7
	export .r8
	export .r9
	export .r10
	export .r11
	export .r12
	export .r13
	export .r14
	export .r15
	endsection

; Jump to the main function, then exit(0)
	section text
main	extern
__start
	export __start
	jsr main
        lda #0
        swi2

printint
        export printint
        lda #1
        swi2
	rts

exit
	export exit
        lda #0
        swi2
