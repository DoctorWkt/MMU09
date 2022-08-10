# MMU09 - A 6809 single-board computer with an MMU

*This is a work in progress! Don't build anything using this stuff yet.*

I'm working on a 6809 single-board computer with an external MMU, so I can
port or write a Unix-like operating system for it.

The `Kicad` folder has the hardware version of the system, as yet unbuilt nor
tested. The main components are: the 6809, 32K ROM, 512K RAM, a dual UART, a
CF card for storage, a real-time clock and a CPLD to implement addressing and
the MMU.

The `Docs` folder has details of the design and the implementation, along with
a journal that I'm keeping along the way.

The `Verilog` folder has the Verilog code for the CPLD as a Quartus II project.
It also has a Verilog implementation of the 6809, ROM, RAM and a UART so I can
test the MMU to ensure that it will work before I build the hardware.

I haven't found a good C compiler for the 6809 yet. Given that I've already written
my own "toy" C compiler, I thought that I would try to write a 6809 back-end for it.
The `Compiler` folder holds this.

I'd really like to get some feedback, ideas etc. on this project. I've borrowed
a lot of ideas from other peoples' projects; when I get some time I'll list them
here.
