## Fri 15 Jul 2022 13:16:20 AEST

So I've been thinking about building a 6809 SBC with a decent amount of
memory (e.g. 512K, 1M), some ROM to boot from a CF card, a real-time clock,
a dual UART and an MMU device to map the 64K address space to the ROM, RAM
and I/O devices.

The aim would be to port xv6 over to the board and have an 8-bit Unix-like
system. Possibly also bring over the userland from 2.9BSD, 4.3BSD or FreeBSD.
It would keep me entertained, at least.

I would try to emulate it first, with one or both of a) a simulator in C,
and b) a simulator in Verilog. Then, once I believe that it would work,
I could wire it up in Kicad and see if it works.

## Mon 18 Jul 2022 13:42:17 AEST

The MMU is going to be a 5V FPGA. Damn, I thought the TinyFPGA B2 was 5V
tolerant but it isn't. What a PITA! I might have to go back to the
ATF1504AS CPLD which is 5V and has 32 I/O pins. I'd have to see how much
I could cram in to the device, i.e. how many frame registers - 8? 16? 32?

Thoughts at the moment, given the above.

 - the 8-bit data bus goes to all devices but not the MMU.
 - the MMU gets the top 12 address bits in from the CPU. This allows
   us to decode $FFFx. If this is too many, we could receive the top
   4 address bits from the MMU and use a 16-way AND gate to match the
   middle $xFFx.
 - Assuming 4K pages, the low 12 bits go out from the CPU as the page
   offset. We need 7 bits of output from the MMU to fill in the frame
   number.
 - We need the BA/BS CPU pins as MMU inputs to tell if we're in supervisor
   mode, and we'll need to generate an interrupt for at least a page fault.
   So that's another 3 CPLD pins. We're up to 22 CPLD pins.
 - We are going to need to generate CS and/or OE signals for the ROM, RAM,
   CF and DUART. Let's see what we will actually need.

The ROM will need a CS and OE. The RAM will need a CS, OE and RW. The
latter comes from the CPU. That's now 26 CPLD pins.

The 68681 DUART (from the jberen design) has:

 - the eight data bus lines
 - the low four address lines
 - CS and a R/W line. The CPU provides the R/W, we only need to provide
   the CS line from the MMU.
 - an interrupt line which goes to the CPU.
 - all the GPIO lines which will go towards the MAX232 device.
 - We are up to 27 CPLD pins.

Looking at the CF card interface web page, it says: all you need to hook up
are the chip selects, three address lines, lower eight data lines,
the I/O read/write control signals, and master reset control signal. 
That means we need three MMU outputs: CS, IOWR and IORD. We are up to 30
CPLD pins.

I haven't chosen a real-time clock chip yet. Perhaps the DS12C887+ as it's
cheap and through-hole. It seems to be connected to the 8-bit data bus,
plus four signal pins: CS, RW, AS and DS. The latter alternate between
an address on the bus and data on the bus. The RW signal will come from the
CPU. I think we'll need to generate the other three signals.

So now we are up to 33 CPLD pins and we only have 32 pins. So it's close!!!
There will be a way to bring this count down, I'm sure.

One way is a demux. We have CS lines for: ROM, RAM, DUART, CS and RTC. We
could encode this with 3 lines and use a 3:8 demux. This brings us back
down to 31 pins.

I guess the CF lines (IOWR and IORD) and the RTC lines (AS and DS) can be
shared, as the CS line will control which chip is using these lines. That's
another two CPLD lines removed; down to 29 pins.

## Fri 22 Jul 2022 09:06:56 AEST

I've spent a few days wiring things up in Kicad, mainly to work out what
lines do need to go to the CPLD. Here is the BOM:

 - 6809 CPU
 - ATF1502 CPLD
 - 74HCT30 8-input NAND
 - 74HCT54 dual JK flip-flop
 - 32K EEPROM
 - 512K SRAM
 - 68681 DUART
 - two MAX232s
 - 40-pin CF card adaptor
 - DS12C887 real-time clock
 - sundry passives

This isn't going to fit on a 10cm x 10cm PCB, I don't think. More like
a 15cm square PCB.

Here are the 16 CPLD input signals:

 - eight data bits
 - a clock signal, E for now
 - BS to tell when we are handling an interrupt
 - R/W
 - the top four virtual address bits
 - MID4, which is active low on virtual addresses $xFFx
 

And the 15 CPLD output signals:

 - PGFAULT
 - the top seven physical address bits
 - five chip select lines
 - RTCAS and RTCDS lines

That's just under the 32 available I/O lines, phew. Now we need to work
out how to use them.

## Fri 29 Jul 2022 12:19:23 AEST

I've given up on the idea of using a CPLD. It's too much effort to learn
CUPL and for a specific CPLD device. Instead, I've gone back to using TTL
logic.

I now have four 4x4-bit register files to map 8K pages onto the RAM.
The ROM is enabled for the top 32K when in kernel mode, and for $FFxx
always. The I/O space is $FExx in kernel mode. I'm using a S/R
flip-flop to store the kernel mode. The BS line sets kernel mode,
and the reset line is tripped by a memory access to an I/O location
which, of course, can only be done in kernel mode.

The RAM memory map is shared by the user process and the kernel,
which is a bit of a pain. The kernel will have to map in its own
pages on entry, and map the user pages on exit.

## Sat 30 Jul 2022 09:31:27 AEST

I spent some time yesterday and this morning building Verilog models
for the address decode logic and the MMU. There are way too many TTL
devices for the address decode, sigh. I like the MMU design.
The four 4x4-bit register files are in pairs, one pair for the low 32K
and the other pair for the upper 32K. That's 8 bits of output. I need 6
bits for each page frame. I'm using one of the spare bits as an invalid
bit. It's tied directly to NMI.

But I've just realised that it's likely that the register file values
start at all zeroes. So, to begin with, all RAM is mapped to the low 8K
in the RAM chip, and all access to any of those are going to cause an NMI.
So I guess the ROM has to quickly set up valid page table entries! OK,
I was thinking I had to put in an inverter into the NMI line, but I don't
think so. We boot into the $FFxx range and I can put enough code in there
to set up the page table.

At the moment, apart from the 3:8 I/O decode and the kernel mode flip-flop,
I'm using five other TTL devices for the decode. I'm using an AND and an OR
gate, and not using the spare three on each chip. Sigh. I'd be happy with
some other mapping, but need ROM at $FFFx, and I don't want to put the I/O
in the middle, nor at $000x because that's used for fast memory access
(zero page). I'm also worried about the propagation delay through the gates.

I found out how to set delays on continuous assigns and non-blocking assigns
in Verilog, so I can now see the delays in GTKWave. The clock cycle is 270nS
for a 3.6864MHz clock. We've got to decode ROM, RAM or I/O (i.e. to set the
chip select line), then get data to/from the device from/to the data bus.

About 30nS for ROMCS, 50nS for RAMCS, 40nS for an I/O CS. For RAM, we need to
go through the page table entries which are 15nS. They change at the same time
that the address changes. OK, I think that's not too bad. The RAM has a 55nS
access time, so 105nS overall. That's compared to the 270nS clock cycle. The
ROM is 150nS read access, so I think things should be OK here.

## Sat 30 Jul 2022 17:17:33 AEST

Oops. The 6809 BS line is low but goes high for the two cycles required to
fetch the vector for whatever interrupt: NMI, IRQ, FIRQ or any of the three
software interrupts. So I need to arrange the kernel mode flip-flop to set
to kernel mode when it goes high!! Luckily there's a spare NAND gate which I
can use as an inverter. Question: how many NAND gates am I using as inverters,
and can I use a proper inverter chip instead?

## Mon 01 Aug 2022 08:15:09 AEST

I already use bit 7 of each page table entry to mark invalid pages. I had thought
of using bit 6 to make pages read-only, but I've changed my mind. I'm going to
use bit 6 for copy-on-write pages. So the logic goes like this now:

 - bit 6 OR the R/W line is active low when there's a write on that page.
   Call this line ~{COWFAULT}.
 - ~{COWFAULT} OR bit 7 is active low when there's a COW page write or the
   page is invalid. Call this line ~{PGFAULT}.
 - ~{PGFAULT} OR the KERNEL line is active low when the CPU is in user mode
   and there's a page fault. We tie this to the NMI line of the CPU.

So, the CPU receives either invalid or COW page faults when in user mode only.
What I like is that the actual write to RAM still goes ahead on an invalid
page. Consider the user stack at top of memory:

```
  +----------------------+
  |  top page for stack  |
  +----------------------+
  | guard page - invalid |
  +----------------------+
```

If the process pushes on the stack and writes on the guard page (which is
there but marked invalid), then the write succeeds and the CPU takes an
NMI. We don't need to redo the user mode instruction. We simply mark the
guard page as valid and add another guard page:
  
```
  +----------------------+
  |  top page for stack  |
  +----------------------+
  | next page for stack  |
  +----------------------+
  | guard page - invalid |
  +----------------------+
```
 
It does mean that we have to allocate two pages for the stack, minimum.

Damn, damn. I think that also means that the write to the COW page will
also go ahead, and we won't have the ability to copy it and keep the
version before the write. Oh well, I'll remove all that wiring :-(

## Mon 01 Aug 2022 10:25:33 AEST

I've done the above, added bypass caps and a barrel jack. I've done a
really rough PCB layout, left the bypass caps in a group. Freerouter
does work fine. So I'm getting to the point where it's buildable.
Whether it works or not is another question :-)

## Mon 01 Aug 2022 10:49:56 AEST

I came across this project on the RetroBrew forum by plasmo:
https://www.retrobrewcomputers.org/doku.php?id=builderpages:plasmo:6502:cpld6502:cpld6502r0
It's a SBC which supports several CPUs and uses a EPM7128SLC84 128-macrocell CPLD.
Digikey lists it as obsolete. But he is writing the code for the CPLD in Verilog.

Then I came across this web page with useful ideas on 5V CPLDs: https://avitech.com.au/?page_id=3195.
The page talks about "an old version of Altera’s Quartus II software can be
used to develop in Verilog or VHDL to target the Atmel ATF1508AS device."
Essentially, use Atmel’s “pof2jed” Windows utility to covnvert to the JED
format which can be used to program the CPLD. Digikey lists the ATF1508AS
device as still in stock, as is the ATF1504AS device.
To program in Verilog, download “Quartus II Web Edition” version 13.0sp1.

I did a quick count of the number of I/O pins to implement the current MMU
and decode logic. It's 19 pins in and 12 pins out. That's 31 pins, which
would fit the ATF1504AS device. Not sure if it's got enough internal logic.
It would be nice if it did, as I could have smaller pages, and even generate
signals for two 512K RAM chips :-)

## Mon 01 Aug 2022 13:43:08 AEST

Download the Quartus II software for Linux, installed it. No idea how to use
it at the moment. I could lose the two MAX232s and replace with these things:
https://auseparts.com.au/FT232RL-3.3V-5.5V-FTDI-USB-to-TTL-Serial-Adapter-Module.
It would save some space on the PCB.

There's a Flash chip, SST39SF040, which is 512K, in DIP format and acts like
an EEPROM except that you can write to it while installed. Not sure if it's
useful.

Here's a Quartus II quick start: https://www.fpga4fun.com/QuartusQuickStart.html
I also have POF2JED working in Wine.
It works. Now I need to work out which Altera devices are equivalent to
the Atmel ones. Online, I have:

EPM7032 = ATF1502
EPM7064 = ATF1504
EPM7128 = ATF1508

The Atmel ATF1504AS has 64 macrocells and 44 pins, I think.
Seems to be the same as the EPM7064AE.

I built the ledblink project from the quick start guide,
used one of the EPM7064 devices, and created a POF file.
I then did: $ pof2jed -device 1504as ledblink.pof
and it created the JED file with no problems.

Now I should see how much of the decode & MMU I can fit into the device!

## Mon 01 Aug 2022 17:31:10 AEST

I've just put in the decode Verilog into Quartus and it only uses 28% of
the macrocells. I haven't put in the MMU code yet. That's a good start,
though.

## Tue 02 Aug 2022 09:01:45 AEST

I've added the MMU code and tidied things up a bit, and also added defines
so I can test the code with Icarus Verilog. At present the code passes
the decode tests but I haven't done the MMU tests yet. The code uses too
many macrocells and a few too many pins. So I'll need to do some refinement,
and/or move up to the CPLD that plasmo is using as it has 128 macrocells.
Anyway, it's promising!

## Tue 02 Aug 2022 11:57:18 AEST

I added the MMU tests and fixed a few problems. We're now using 37 pins
and 76 macrocells. I'm going to try altering the `always` commands and
see if that helps. Nope, that didn't change a thing. Neither did tinkering
with the "optimisation" settings.

Given that I'm going up to the next device, let's see what happens when I
use 4K sized pages :-) Wow, the number of macrocells goes up to 166; that's
definitely not going to fit into 128 macrocells. Damn :-(

## Wed 03 Aug 2022 08:31:56 AEST

I brought in the two clock signals to the Verilog code as Quartus was
warning about combinatorial loops. That got rid of the problem but I
haven't fixed the test bed to get the tests to work yet.

I've taken a snapshot of the Kicad project as "Aug 3 07:52 mmu09_in_ttl.tgz".
I've just done a hand drawing of the best pin connections between the 6809,
the RAM and the ATF1508AS CPLD which has 84 pins.

I'm going to put the CPLD into the Kicad design and remove the TTL chips
and see how it goes. Done. I also want to lose the two MAX232 chips and
replace the DB9s with two FT232RL serial to USB daughterboards. That will
bring the chip count down to eight!!

## Wed 03 Aug 2022 12:54:46 AEST

Done. It all fits onto a 10cm x 10cm board now. I've assigned pins to the
CPLD to try and make routing easier. I've laid the board out with the
bypass caps, and `freerouter` is doing its thing now.

What I think I should do next is bring in a 6809 Verilog model, some RAM
and ROM, and just wire up a basic board with 32K RAM, 32K RAM, no UART.
Just to get the CPU working. Then I can add in a UART with some basic mapping,
and get that to work. With this done, I can try bringing in the decode/MMU code.

## Thu 04 Aug 2022 07:59:49 AEST

OK, I've just rearranged the directory structure with the Verilog code in
alongside the Quartus project, and the Kicad stuff in a Kicad/ folder.
I had a look at the 6809 cycle accurate Verilog code yesterday, looks like
it should be straightforward to get it to work. I'll need to make a top-level
file, but I can add ROM, RAM, UART etc. Guess I'll have to learn 6809
assembly language now!!

## Thu 04 Aug 2022 10:47:15 AEST

Yay. I've built a Verilog 6809 SBC with 32K ROM and 32K RAM, no UART yet.
I've assembled my first 6809 code, just NOPs and a JMP. It loads into the
SBC and runs fine! Very happy now.

## Thu 04 Aug 2022 15:41:05 AEST

I quickly mapped the UART to ROM location $FFF1 and, after a bit of fixing, I
can now send output. Fantastic. I've also set up the testbed so that a memory
access on $FFF0 halts the simulation.

So now I have a 6809 SBC with ROM, RAM and UART which seems to work. I've
had to write a small script to take the `a09` binary output and convert it
into the hex format that Verilog expects for the ROM image. `hd` to the rescue!

## Thu 04 Aug 2022 18:01:12 AEST

I've added the MMU & decode Verilog device to the system, and now instead
of writing "wkt\n" to the output, I'm getting "kt\n". Ah, the "w" is becoming
0x00. So the RAM read/write isn't working correctly. OK, at least I can try
to diagnose that. It's a good start.

Ah, the page table is undefined until I put values in. So I've put an entry
in for page zero and now I get the full output, so that's a win!!

I can also switch to user mode, but the code has to be in the $FFxx area
as the ROM gets switched out otherwise :-)

So it's early days but it feels like the decode and MMU both work. I guess
I need to try going from user mode back to kernel mode, and also take a
page fault and see what happens. To do both, I'll have to copy instructions
from ROM down to a RAM page and then jump there.

What I should do first is learn some more 6809 assembly language!

## Fri 05 Aug 2022 08:32:59 AEST

I read some stuff on 6809 assembly language last night. I also put in code
to do a syscall with SWI, and also to access an invalid page. The latter
works, but the 6809 Verilog code doesn't raise BS when we do the SWI instruction,
and so we don't go into kernel mode. Damn.

## Fri 05 Aug 2022 11:14:49 AEST

I've had an e-mail conversation with Greg Miller who wrote the 6809 Verilog code.
He's going to get back to me after the weekend with an answer. For now, I'm
going to assume that BS should go high for any SWI instruction, and I'll hand-edit
his code and see how it goes.

I'm looking at the timings. For an NMI (page fault), the registers are pushed
onto the stack before BS goes high. This means that we are in user mode for
the saving of the CPU state, then go into kernel mode. Question: what happens
when the stack moves down into the invalid (guard) page. This will cause a
page fault. Will the pushing of the registers onto the "invalid" page cause
successive NMIs to occur? I'll have to try and find out.

Yes, it seems that way. I should be able to signal the page fault and turn
kernel mode on at the same time. But that's a problem. If the invalid page
is in the upper 32K, then it will masked by ROM once kernel mode is enabled
and so we won't be able to save the registers. Damn!

Can I set an internal bit register to stop the page fault after a clock
cycle, and then clear it once we get into kernel mode? Yes, I've done that.
I just realised that we have to make that page valid before we can RTI,
as on the way back we access the page in user mode to pull regs off the stack
which causes another NMI :-)

OK, I changed the 6809 Verilog code to set BS on SWI instructions and tried it.
I forgot that I'd left the stack just above the invalid page. So the SWI caused
the state to be saved, which caused the NMI to happen. But both got stacked
properly on the stack, the page was made valid by the NMI handler, and we
eventually made it back to user mode :-) I'll try it again with the stack
starting a bit higher! Yes, that's good; we handle the SWI as expected.

## Fri 05 Aug 2022 14:03:00 AEST

I downloaded the `cmoc` compiler, the assembly output is weird! It's also
in C++, so it won't self-compile unfortunately. Maybe I'll have to write a
6809 backend for my compiler?!

Another question. Should I keep the DUART and use its built-in timer, or
should I use the FT238R device (as per CSCv2) and the timer in the RTC?
I suppose I'll get some GPIO pins with the DUART.

I tried to install Verilator from the Debian package but I couldn't compile
the basic example. So I've downloaded from Github and compiling it from scratch.
Done. Now the problem is that it gives lots of warnings when I give it the
mmu09_sbc.v file. So, for now, I'll work on other things.

## Fri 05 Aug 2022 14:52:27 AEST

Given that I want to build a Unix-like operating system for the board,
what are my options?

 - xv6, written in C, pretty cut down but I've already ported some of the
   BSD libraries and userland to it.
 - One Man's Unix: https://github.com/EtchedPixels/OMU/tree/master/omu09/omu09
   This was written for a 6809 board a long time ago. In C.
 - FUZIX: https://www.fuzix.org/. It says: SYS3 to SYS5.x world with bits of
   POSIX thrown in for good measure. In C. However: On a 6809 it's just about
   possible to run in a straight 64K of ROM.
 - UZIX: http://uzix.sourceforge.net/uzix1.0/index.php?lang=us.
   
## Sat 06 Aug 2022 10:44:24 AEST

There's a nice 6809 emulator written by Arto Salmi on Github. It has
breakpoints and a monitor, it's in C and it looks to be easily modifiable.
I should be able to change it to have the user/kernel mode, the MMU and
address decoding.

I'm also thinking that, perhaps, I should write a 6809 backend for `acwj`
as I don't like the other compilers much.

## Sat 06 Aug 2022 13:43:43 AEST

I've modified the 6809 emulator to have the user/kernel mode, the MMU and
address decoding. I'm sure I've slowed it down a lot with the vadr -> padr
mapping on each memory access, but it runs the code I've written for the
Verilog version fine. It doesn't do NMI yet, and that's only going to happen
on a page fault, so I hope I can do that in a deterministic way. Maybe set
an NMI flag and, at the end of each instruction check it and force the NMI
action.

## Sat 06 Aug 2022 14:26:08 AEST

I've just done the NMI handling and checked that it works OK. This was with
the user mode pushing onto the stack which crosses over onto the invalid page.
So, the simulator in C now seems to match the Verilog version, yay! I was thinking
that, if/when I write the `acwj` backend, I need a way to test that the compiler
produces the right code.

What I might also do is to set up some SWI system calls, e.g. printf(). So I can
keep the printf()s in the `acwj` test code and I won't have to implement a printf()
in the 6809 simulator.

## Sun 07 Aug 2022 11:51:04 AEST

I had a look at the LW assembler, looks fine. So maybe I should reconsider the `cmoc`
compiler. I can still add in some SWI systems to the C simulator and try it out.

## Tue 09 Aug 2022 08:34:09 AEST

I don't like `cmoc`, maybe because of the look of it's output. So I'm redoing `acwj`. This time with an intermediate representation. I've got
most of it done, but no 6809 code generation as yet.

## Tue 09 Aug 2022 10:42:12 AEST

OK, so the new compiler is mostly printing out good intermediate code.
I've taken the salmi simulator, made it start at $100 and use SWI2 as
an Apout-like syscall handler. So far there is exit() and printint(D).

## Tue 09 Aug 2022 14:28:48 AEST

So, the `acwj` compiler now spits out enough 6809 code that I can compile,
assemble, link and execute a binary. I have a `crt0.s` and a `libc.a`
with `printint()` in it. The output is wrong; that's because the compiler
is creating byte literals but I'm trying to print 16-bit words. But it's
a good start.

I can manually cast (int)23 to get a 16-bit literal and that works. I still
have to work out why the compiler isn't casting to function call arguments.

## Thu 11 Aug 2022 12:22:31 AEST

I'm now manually changing the type of INTLIST AST nodes to match the type
of a function's parameter. Ugly but it works. I can now compile and run
a C program with no hand twiddling.

The commands are:

```
./cwj -S fred.c
lwasm -f obj -o fred.o fred.s
lwlink -f srec --section-base=text=0x100 --section-base=registers=0xc0 -L /home/wkt/wktcloud/MMU09/Compiler/lib -o fred fred.o -lc
um6809 -b 0 fred
```

I also threw up all I've done so far into a Github repository, so others
can have a look at it.

## Thu 11 Aug 2022 13:24:16 AEST

I'm getting frustrated with my compiler, so I'm going back to `cmoc`.
I've been able to compile a program, link it with my `crt0.s` and run
on `um6809`. But more work to get it done with no hand waving.

Yay, I'm getting somewhere. I had to set up the `SYNC` instruction in the
user-mode simulator to do `exit()`. Now, this all works. Firstly, `fred.c`:

```
extern ), 
extern ), 
extern ), 

int a;
char b;

), 
{ a= 5; printint(a );
  myexit();
  b= 7; printint(b);
}
```

Now the `crt0.s`:

```
        section code

_printint
        export _printint
        lda #1
        swi2
        rts

_myexit
        export _myexit
        sync			; Both the SYNC
        lda #0
        swi2			; and the SWI2 work, don't need both
        rts
```

And the compile line:

```
cmoc -nodefaultlibs --org=100 --srec  -o fred fred.c -L lib -lc
```

I have to define my own `exit()` as the one that `cmoc` uses should execute `SYNC`
but I can't make it happen yet.

## Thu 11 Aug 2022 14:31:50 AEST

Getting closer. If I do this:

```
cmoc -nodefaultlibs --org=100 --usim --srec  -o fred fred.c -L lib -lc
```

then I can have this `fred.c`:

```
extern ), 
extern ), 

int a;
char b;

int main()
{ a= 5; printint(a);
  b= 7; printint(b);
  return(0);
}
```

The `cmoc` startup code does a heap of stuff that I don't think I need.
So I need to find a way to define a new `cmoc` target, and then `ifdef`
the startup code to only do what I need.

## Fri 12 Aug 2022 08:01:15 AEST

I changed the `crt.asm` code to just be this:

```
    IFDEF _CMOC_MMU09_TARGET_
_main                   IMPORT
_exit                   EXPORT
INILIB                  EXPORT
INILIB
    JSR _main
_exit
    LDA #0
    SWI2
```

and I changed `cmoc` itself to have MMU09 as the default platform, so
I can now do this compile command: `cmoc -o fred fred.c -L lib -lc`

What I want to do now is to do a full 'Apout' to the user-mode simulator,
so I can run binaries with argv[] etc. and with no initial breakpoint.
Then, take a userland like 2.11BSD cmds+lib and build it. I'll use 2.11BSD
as it's a 16-bit system.

## Fri 12 Aug 2022 11:33:36 AEST

I've started on the simulator with `open()`, `close()`, `read()` and `write()`.
For now, no rewriting of the pathnames like Apout. And I'll do the argv[] stuff
later. But it's good.

I had a look at the 2.11BSD libs, and I think I'd prefer to work with the libs
that I added to `xv6`. Also, I know what syscalls `xv6` has:

```
chdir(), close(), dup(), exec(), exit(), fork(), fstat(), getpid(), kill(), 
link(), mkdir(), mknod(), open(), pipe(), read(), sbrk(), sleep(), unlink(), 
wait(), write(), uptime(), lseek(), ioctl(), time(), fchdir() 
```

So if I can implement these in `um6809`, then I should be able to port the
`xv6-freebsd` libs and commands over to this system.

## Sun 14 Aug 2022 10:51:38 AEST

I had a bit of a fight with adding argc, argv[] and envp[] to the system.
Anyway, I've got it working now. Needed to change the `crt.asm` again, and
patch `cmoc` to allow envp[].

I've implement most of the above syscalls, the ones that were trivial. Now
I've got to do things like `exec()`, `pipe()` etc. Not sure how to do `sbrk()`
at the moment, as I don't think the output format from `lwlink` has details
of the end of the bss.

## Sun 14 Aug 2022 18:18:28 AEST

I've been trying to port the xv6-freebsd libraries over. It's been painful
with `cmoc` giving lots of warnings and errors. I'm now looking at Fuzix,
as there is already a 6809 port of the OS. Now I'm trying to build gcc6809.
I'm using the information in this Docker file:
https://github.com/amaiorano/gcc6809-docker/blob/master/Dockerfile

Essentially:

```
git clone https://gitlab.com/dfffffff/gcc6809.git
cd gcc6809/build-6809
sudo make everything
```

## Mon 15 Aug 2022 12:12:31 AEST

I got `gcc6809` to build and install, and it seems to work OK. I've downloaded
Fuzix, and I can build the libraries with it which is a start. But, when trying
to build any of the applications, the linker complains about the format of
the `abort.o` file, which starts with:

```
XH2
H 3 areas 7 global symbols 2 banks 1 modes
M abort.c
G 00 00 80 81 82 83 84 85 86 87 88 89 8A 8B 8C 8D 8E 8F
B _CSEG base 0 size 0 map 0 flags 0
B _DSEG base 0 size 0 map 0 flags 4 fsfx _DS
```

I don't recognise that, and `lwlink` (which Fuzix uses as the linker) also
doesn't recognise it. So, either `gcc` is spitting out the wrong object format,
or I'm using a version of `lwtools` that doesn't recognise it. I've joined the
Fuzix Google group and asked about it there. However, the group seems very quiet,
so I'm not holding my breath on an answer. Sigh.
