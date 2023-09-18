run: monitor.rom
	iverilog icarus_tb.v
	vvp a.out

monitor.rom: monitor.asm

	a09 -l monitor.lst monitor.asm
	./makerom monitor monitor.rom
	rm monitor

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

clean:
	rm -rf *.out *.vcd v09.rom monitor monitor.rom monitor.lst \
		mmu_decode.edif mmu_decode.fit mmu_decode.io \
		mmu_decode.jed mmu_decode.pin mmu_decode.tt3 mmu.log \
		mmu_decode.svf
