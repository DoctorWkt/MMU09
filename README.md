# MMU09 - A 6809 single-board computer with an MMU

*This is a work in progress! Don't build anything using this stuff yet.*

I'm working on a 6809 single-board computer with an external MMU, so I can
port a Unix-like operating system for it, probably xv6 or Fuzix.

The `Hardware` folder has the hardware version of the system, as yet unbuilt nor
tested. The main components are: the 6809, 32K ROM, 512K RAM, a dual UART, a
CH375 device for storage, a real-time clock and an ATF1508 CPLD to implement
address decoding and the MMU.

The `Docs` folder has details of the design and the implementation.

The `Verilog` folder has the Verilog code for the CPLD as a Quartus II project.
It will eventually have a Verilog implementation of the 6809, ROM, RAM and a UART
so I can test the MMU to ensure that it will work before I build the hardware.

I'd really like to get some feedback, ideas etc. on this project. I've borrowed
a lot of ideas from other peoples' projects; when I get some time I'll list them
here.
