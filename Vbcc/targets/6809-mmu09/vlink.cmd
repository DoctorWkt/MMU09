MEMORY
{
  ram :  org = 0x0002, len = 0xFDFE
}

SECTIONS {
	.text: { *(.text) } > ram
	.dtors : { *(.dtors) } > ram
	.ctors : { *(.ctors) } > ram

	. = MAX(. , 0x2000);

	.rodata: { *(.rodata) } > ram
	.dpage: { *(.dpage) } > ram
	.data : { *(.data) } > ram
	.bss (NOLOAD) : { *(.bss) } > ram

	__BS = ADDR(.bss);
	__BL = SIZEOF(.bss);
	__ENDBSS = ADDR(.bss) + SIZEOF(.bss);
	__DS = ADDR(.data);
	__DC = LOADADDR(.data);
	__DL = SIZEOF(.data);
	__STACK = 0xFDFD;
}
