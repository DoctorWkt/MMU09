MEMORY
{
  ram  :  org = 0x0000, len = 0x2000
  jump :  org = 0x2000, len = 0x0030
  rom  :  org = 0x2030, len = 0x5ED0
 trom  :  org = 0xFF00, len = 0x00F4
  vect :  org = 0xFFF4, len = 0x000C
}

SECTIONS {
	.jumptable: { *(.jumptable) } > jump
	.text: { *(.text) } > rom
	.dtors : { *(.dtors) } > rom
	.ctors : { *(.ctors) } > rom
	.rodata: { *(.rodata) } > rom
	.toprom: { *(.toprom) } > trom
	.vectors: { *(.vectors) } > vect
	.dpage: { *(.dpage) } > ram
	.data: { *(.data) } > ram
	.bss (NOLOAD) : { *(.bss) } > ram

	__BS = ADDR(.bss);
	__BL = SIZEOF(.bss);
	__DS = ADDR(.data);
	__DC = LOADADDR(.data);
	__DL = SIZEOF(.data);

	__STACK = 0x7FFD;
}
