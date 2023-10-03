monitor.rom: monitor.bin
	./makerom monitor.bin monitor.rom

monitor.s19: monitor.asm
	asm6809 -S -o monitor.s19 -l monitor.lst monitor.asm

monitor.bin: monitor.s19
	srec_cat monitor.s19 -Motorola -output temp -Binary
	dd if=temp bs=32768 skip=1 of=monitor.bin
	rm -f temp

user.s19: user.asm
	asm6809 -S -o user.s19 -l user.lst user.asm

clean:
	rm -rf monitor.rom monitor.lst monitor.bin monitor.s19 \
		user.s19 user.lst
