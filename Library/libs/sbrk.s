	.text

; Check the value in _brktmp. If it's below the end of the bss, or too
; close to the stack. If it is, return D as -1. Otherwise, put the current
; brk value into D, set the current brk value to _brktmp and return the
; old brk value in D.

_checkbrk:
	cmps	_brktmp		; How close is _brktmp to the SP?
	bls	_badcheckbrk	; At or above, very bad
	ldx	_brktmp		; How close is _brktmp to the end of bss?
	cmpx	_initbrkval
	blo	_badcheckbrk	; Below, very bad
				; The _brktmp value is OK, so ...
	ldd	_brkval		; Get the old brk value into D
	ldx	_brktmp		; Copy the new brk value into _brkval
	stx	_brkval
	rts			; and return the old brk value
_badcheckbrk:
	ldd	#0xffff		; Error, so return -1
	rts

; brk(): set the current brk value given the unsigned value in the D register.
; Check the proposed value before setting the new brk value.
; Return 0 if OK, or -1 on failure.

brk:
	.global	brk
	std	_brktmp		; Save the proposed value in _brktmp
	jsr	_checkbrk	; Now check that it's a sensible value
	cmpd	#0xffff		; Was it a bad value?
	beq	_badbrk		; Yes
	clra			; Value was OK, so return 0
	clrb
_badbrk:
	rts			; If we took the _badbrk branch, D is -1


; Increase/decrease the current brk value given the signed change in the D reg.
; Check the proposed value before setting the new brk value. Return the old
; brk value on success, or -1 on failure.

sbrk:
	.global	sbrk

	addd	_brkval		; Add the change to the current brk value
	std	_brktmp		; Save it temporarily
	jsr	_checkbrk	; Now check that it's a sensible value
	rts			; Return either old brk value or -1

	.data

; _brkval is the current brk value, initially __ENDBSS
; _initbrkval is always the initial end of bss
_brkval: 	.word	__ENDBSS
_initbrkval: 	.word	__ENDBSS
_brktmp: 	.word	0
		.global	_initbrkval
