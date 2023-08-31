## Verilog Version of MMU09

This folder has the Verilog code for the CPLD in the file `mmu_decode.v`. The
Makefile has a workflow set up using https://github.com/hoglet67/atf15xx_yosys
to create the JEDEC file `mmu_decode.jed`.

There is also a partial simulation of the whole SBC using Icarus Verilog.
It's enough to do primitive UART output, test the RAM, ROM and the MMU's
operation. The simulation runs the assembly code in `monitor.asm`.
