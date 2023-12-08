	.text

swi2call:
	swi2
	stx	errno
	rts

_exit:
	.global exit
	.global _exit
	ldx #0x00
	jmp swi2call


romgetputc:
	.global romgetputc
	ldx #0x02
	jmp swi2call

romputc:
	.global romputc
	ldx #0x04
	jmp swi2call

; Slightly different arguments than Unix:
; void sys_exec(int argc, char *argv[])
sys_exec:
	.global sys_exec
	ldx #0x06
	jmp swi2call

chdir:
sys_chdir:
	.global chdir
	.global sys_chdir
	ldx #0x08
	jmp swi2call

close:
sys_close:
	.global close
	.global sys_close
	ldx #0x0a
	jmp swi2call

dup:
sys_dup:
	.global dup
	.global sys_dup
	ldx #0x0c
	jmp swi2call

sys_fstat:
	.global sys_fstat
	ldx #0x0e
	jmp swi2call

link:
sys_link:
	.global link
	.global sys_link
	ldx #0x10
	jmp swi2call

lseek:
sys_lseek:
	.global lseek
	.global sys_lseek
	ldx #0x12
	jmp swi2call

mkdir:
sys_mkdir:
	.global mkdir
	.global sys_mkdir
	ldx #0x14
	jmp swi2call

open:
sys_open:
	.global open
	.global sys_open
	ldx #0x16
	jmp swi2call

read:
sys_read:
	.global read
	.global sys_read
	ldx #0x18
	jmp swi2call

unlink:
sys_unlink:
	.global unlink
	.global sys_unlink
	ldx #0x1a
	jmp swi2call

write:
sys_write:
	.global write
	.global sys_write
	ldx #0x1c
	jmp swi2call

_tcattr:
	.global _tcattr
	ldx #0x1e
	jmp swi2call

fork:
	.global fork
	ldx #0x20
	jmp swi2call

wait:
	.global wait
	ldx #0x22
	jmp swi2call

getpid:
	.global getpid
	ldx #0x24
	jmp swi2call

; This syscall only uses the first argument: the pid.
; Any other argument is ignored and treated as SIGKILL.
kill:
	.global kill
	ldx #0x26
	jmp swi2call

pipe:
	.global pipe
	ldx #0x28
	jmp swi2call
