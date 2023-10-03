# MMU09 - A 6809 single-board computer with an MMU

*This is a work in progress! Don't build anything using this stuff yet.*

I'm working on a 6809 single-board computer with an external MMU, so I can
port a Unix-like operating system for it, probably xv6 or Fuzix.

The `Hardware` folder has the hardware version of the system, as yet unbuilt nor
tested. The main components are: the 6809, 32K ROM, 512K RAM, a dual UART, a
CH375 device for storage, a real-time clock and an ATF1508 CPLD to implement
address decoding and the MMU.

The `Docs` folder has details of the design and the implementation.

The `Verilog` folder has the Verilog code for the CPLD as well as
a Verilog implementation of the 6809, ROM, RAM and a UART. This allows me to
test the MMU to ensure that it will work before I build the hardware.

I'd really like to get some feedback, ideas etc. on this project. I've borrowed
a lot of ideas from other peoples' projects; when I get some time I'll list them
here.

## Status - 1st September 2023

I've updated the schematic with the pin headers to make the PCB into a "hat"
which will sit on top of an Atmega 2560. I'll be able to use this to monitor
and partially control the SBC.

I've made a first attempt at laying out the components on a PCB, and there
is an archive of the Kicad project in the `Hardware` folder.

In the `Verilog` folder, there is now an Icarus Verilog simulation of the
system. I can run 6809 instructions and see the system go in/out of kernel
mode, and see the page mappings change.

The CPLD Verilog code now has reset functionality.

## Status - 10th September 2023

I now have a completed PCB design; see the `Hardware` folder for a schematic
and views of the PC board.

## Status - 18th September 2023

I have ported the xv6 filesystem code from [https://github.com/DoctorWkt/Nine-E/].
This gives me a Unix-like environment but with only one program running at a time.
I've used the address decoding on the MMU09 to give each program 63.5K of address
space. This works in the Salmi simulator.

I've ordered the PCB and the parts to build the hardware; they haven't arrived yet, sigh.

## Status - 4th October 2023

I've built the board and, with the exception of the DS12C887, everything
works at 14.75MHz. I've changed the CPLD code to have a stack of four
previous user/kernel modes; this allows us to go from user mode to kernel
mode, followed by two nested interrupts, and return safely back to user mode.

Programming the CPLD with the Verilog -> Yosys -> JED workflow is nice and
easy, using the workflow from [https://github.com/hoglet67/atf15xx_yosys].
I'm then using the `jed2svf` converter from
[https://whitequark.github.io/prjbureau/tools/fuseconv.html] to make the
SVF file, and a FT232H device and OpenOCD to send the SVF file to the ATF1508 CPLD,
based on the information from
[https://www.hackup.net/2020/01/erasing-and-programming-the-atf1504-cpld/].

I have a simple monitor which allows me to dump memory contents,
alter memory and run a program in user mode. There are syscalls to read from
& write to the UART, and to exit back to the monitor.
