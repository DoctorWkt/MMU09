# Design of the MMU09 Single-board Computer

I want to build an SBC with a 6809, as it's a nice CPU. I also want to have
a machine that can run a Unix-like operating system, and protect the kernel
and processes from other processes.

To that end, this design has an MMU in a CPLD that sits between the 6809
and the memory & I/O devices. Here are the basics on how it will work.

## User/Kernel Mode and Memory Layout

The CPLD implements a user/kernel (unprivileged/privileged) mode bit.
This is flipped to kernel mode when the 6809's `BS` line goes high
(when the 6809 is loading the vector to handle a reset, an interrupt or SWI
instruction).

In user mode, the whole 64K of address
space is RAM except the top 256 bytes at `$FF00 - $FFFF` which is ROM.

When in kernel mode, addresses `$FE00 - $FEFF` become an area for
performing I/O and manipulating the page table entries. And 
24K of ROM gets mapped in from addresses `$2000 - $7FFF`.

This memory layout provides each process with 63.5K of RAM memory and prevents
processes from doing I/O or changing the page table entries directly. It also
allows the kernel to reside in 24K of ROM. Only the kernel in ROM can access
the I/O devices because they only get memory-mapped when in kernel mode.

![address map table](addressmap.png)

A user process can transition to kernel mode by executing one of the `SWI`
instructions. This causes the CPU to jump to the handler for the relevant
instruction which is in the top 256 bytes of memory. This enables the 256
byte I/O area at `$FE00 - $FEFF` and the 24K of ROM at `$2000 - $7FFF`.

To get from kernel mode back to user mode, the operating system jumps up to
the top 256 bytes of ROM. From here, it can hide the I/O area and 24K ROM by
tickling I/O memory location `$FE60`. Then the operating system can return
from interrupt and go back to the code running in user mode.

## Hiding the 24K ROM

It's useful to be able to hide the 24K of ROM when we are kernel mode.
For example, perhaps the user program wants to write data out to the
storage device, but the program has this data in a buffer starting at
`$C000`. With the 24K ROM mapped in, this data is hidden.

In the I/O area, tickling address `$FE50` maps out the 24K of ROM
and tickling address `$FE51` maps the 24K of ROM back in. To use this
effectively, there is a modified `memcpy()` routine in the top 256 bytes
of RAM (always visible) that maps the 24K ROM out, does the data copy
with the user data, and maps the 24K ROM back in.

## Stacking User/Kernel Modes

One problem is that, when an interrupt routine is exiting, it doesn't
know if to go back to user mode or to stay in kernel mode. An example
is the UART interrupt handler which is started when a character arrives
from the keyboard. Before the interrupt handler started, we might be
in user mode running the user process. Or, we might be in kernel mode
handling a system call like `read()`.

To solve this problem, the CPLD keeps track of the last four user/kernel
mode states. Just before an interrupt handler returns (with the `RTI`
instruction), it can tickle the I/O address `$FE80`.  This tells the
CPLD to go back to the previous mode, be it kernel mode or user mode.
Thus, we can stack a user mode, a system call and two nested interrupts
and return back from each one safely.

## The MMU

The CPLD also implements an MMU with eight 8K pages. I'd like more pages and
smaller pages, but I can only implement eight page table entries in the CPLD
device that I've chosen.

Each page table entry is eight bits. The lower six bits provide the page frame
number: thus, there are sixty-four 8K frames which allows the MMU to address
512K of RAM. The top bit indicates if the page is "valid". More on this later.

Obviously, one bit in the page table entry is unused. If possible, I'd create
sixteen 4K pages and have 128 page frame numbers, i.e. still 512K. Otherwise,
I could keep 8K pages and this would let the MMU address 1M of RAM.

## Page Mapping

The MMU only controls RAM mapping. In kernel mode, the 24K of ROM is
always contiguous from `$2000` to `$7FFF`. But any visible RAM gets
mapped through the MMU.

When the CPU tries to access visible RAM at an address, the bottom
thirteen bits of the address act as the offset into the page and these
go directly to the RAM device.  The top three bits go into the MMU and
select one of the eight page table entries stored there. The low six
bits of the entry are combined with the thirteen bits of page offset to
create a 19-bit physical address which goes to the 512K RAM device.

In kernel mode, the eight page table entries can be modified by writing to one
of eight addresses in the I/O area. Thus, the kernel can set the RAM mapping for
any process in user mode. Each process can thus only see its own RAM and none of
the memory allocated to other processes.

## Invalid Pages

When a page is marked "invalid" (by the top bit in a page table entry), the page
can still be read or written. However, the CPLD sends an `NMI` to the CPU on
the access, which is picked up by the kernel. So, what use is this?

Imagine there are three user-mode processes on the system, each with one 8K page
starting at `$0000` for machine code and data and another 8K page at `$E000` for
a stack. We have thus allocated six of the sixty-four RAM pages. Let's now set
aside one more page (e.g. page frame 23), mark it as invalid and map it into the
address space of all three processes at `$C000`. This will be a *guard page* below
the three stack pages.

The first process to grow their stack down below `$E000` will cause an NMI. The
kernel is started up and sees this access. The kernel can now allocate page frame
23 to the process that performed the access, and mark it as valid. We now choose
another spare page frame (e.g. frame number 31), mark it as invalid, and map it
below the stack of all three processes.

So, instead of twenty-four 8K pages to completely fill all three processes'
address space, we started with six allocated pages plus one guard page, and
we now have seven allocated pages plus one guard page. This economy of memory
allocation will allow the system to have many processes in memory concurrently.

## CPLD Implementation

I'm writing Verilog to implement the MMU and the addressing of the ROM, RAM and
I/O devices. Have a look at the `mmu_decode.v` file in the `Verilog` folder
to see the CPLD code.
