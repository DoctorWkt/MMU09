run: monitor.rom
	iverilog icarus_tb.v
	vvp a.out

monitor.rom: monitor.bin
	./makerom monitor.bin monitor.rom

monitor.s19: monitor.asm
	asm6809 -S -o monitor.s19 -l monitor.lst monitor.asm

# We have a 32K ROM component. The low 8K isn't used.
# The remaining 24K is mapped from $2000 to $7FFF.
# However, the top 256 bytes of this is also mapped
# at $FF00 to $FFFF. So we build the whole 64K of address
# space, then use "dd" to select out the sections we need.

monitor.bin: monitor.s19
	srec_cat monitor.s19 -Motorola -output temp -Binary 2> /dev/null
	dd if=temp bs=256 count=127 of=monitor.bin
	dd if=temp bs=256 skip=255 count=1 >> monitor.bin
	rm -f temp

test: tb_mmu_decode.out
	vvp tb_mmu_decode.out

tb_mmu_decode.out: tb_mmu_decode.v mmu_decode.v
	iverilog -o tb_mmu_decode.out tb_mmu_decode.v

mmu_decode.jed: mmu_decode.v
	run_yosys mmu_decode > mmu.log
	run_fitter mmu_decode -preassign keep \
		-tdi_pullup on -tms_pullup on \
		-output_fast off -xor_synthesis on $*

mmu_decode.svf: mmu_decode.jed
	jed2svf mmu_decode.jed mmu_decode.svf

mmu_decode.xsvf: mmu_decode.svf
	./svf2xsvf.py mmu_decode.svf mmu_decode.xsvf

clean:
	rm -rf *.out *.vcd v09.rom monitor monitor.rom monitor.lst \
		monitor.bin monitor.s19 \
		mmu_decode.edif mmu_decode.fit mmu_decode.io \
		mmu_decode.jed mmu_decode.pin mmu_decode.tt3 mmu.log \
		mmu_decode.svf mmu_decode.xsvf
