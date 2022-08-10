run: monitor.rom
	iverilog icarus_tb.v
	vvp a.out

monitor.rom: monitor.asm

	a09 monitor.asm
	./makerom monitor monitor.rom
	rm monitor

test: tb_mmu_decode.out
	vvp tb_mmu_decode.out

tb_mmu_decode.out: tb_mmu_decode.v mmu_decode.v
	iverilog -o tb_mmu_decode.out tb_mmu_decode.v

clean:
	rm -rf *.out *.vcd v09.rom monitor monitor.rom
