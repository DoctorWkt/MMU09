monitor.s19: monitor.asm
	asm6809 -S -o monitor.s19 -l monitor.lst monitor.asm

monitor.rom: monitor.s19
	srec_cat monitor.s19 -Motorola -output temp -Binary
	dd if=temp bs=32768 skip=1 of=monitor.rom
	rm -f temp

clean:
	rm -f *.rom *.s19 *.lst temp
