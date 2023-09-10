# Other Design Ideas

*This was written with the SBC design as at 1st September, 2023.*

The current physical design of the MMU09 SBC with the CPLD right
in-between the CPU and the other devices (RAM, ROM, UART, RTC and
CH375 block device) allows for a lot of scope for future development
without any actual rewiring or hardware redesign.

Also, the CPLD's macrocell usage is only at 60% so it has quite a lot
of logic elements left to be used.

Here is a list of some possible design ideas that I've had.

## NULL Dereferencing

We should be able to generate a page fault on a NULL pointer reference, i.e.
an access to location zero.

## Hide the ROM in Kernel Mode

It's nice to have nearly 64K of address space for each process in user-mode
but, at some point, the process will make a system call to do I/O. This
will probably result in the kernel having to copy from a user-mode buffer
into a kernel buffer or vice versa.

Assuming that we will have to map in a frame for kernel data, and the 32K
ROM will replace the top-half of the process' memory, it means that we
will have to map user-mode pages in at difference locations, work out what
the difference is, and then copy from the remapped pages to/from kernel
buffers. This will end up being quite painful to get right.

But, there is always ROM in the top 256 bytes of the address space. So,
can we set up a CPLD toggle to hide the 32K ROM (except the top 256 bytes of
ROM) even when we are in kernel mode?

The idea goes as follows. Let's always start a process' RAM space at the
8K ($2000) mark, even if the code is much smaller than 8K. When we switch into
kernel mode, we map in 8K of RAM from $0000 to $1FFF for kernel buffers and
other variables. This ensures that there won't be any user data that is
hidden by the kernel memory.

When we need to copy between user and kernel buffers, we jump to ROM code
in the top 256 bytes ($FFxx). There will be a routine here to:

 - hide the 32K ROM and expose all of the process' data,
 - copy directly between the user data and the kernel buffers,
 - restore the visibility of the 32K ROM, and
 - return back to the kernel in the 32K ROM.

This makes it so much easier to copy between user and kernel spaces.
The drawback is that we lose up to 8K of data space for each process.

## ROM Size

There is no physical (hardwired) reason why we have to have 32K of ROM
in kernel mode. The CPLD can be programmed to map in any amount of ROM:
8K, 12K, 16K, 24K etc.

Given the previous idea, this may not be necessary, but we can do it
if required.

## Reading Data from the CPLD

Dave's MMU code
at https://github.com/hoglet67/atf15xx_yosys/blob/master/examples/mmu/mmu.v
shows that the CPLD can do tri-state output on the data bus. So, we could
add logic to read the page table entries at the addresses we use to write
them.

What else could we read from the CPLD? Perhaps a condition code register:

 - which device sent the IRQ: the UART or the CH375?
 - what was the reason for the pagefault: an invalid page reference,
   a NULL dereference?
 - any other information that the CPLD might gather?

## The Spare Bit in the Page Tables

Right now we use six bits in each page table entry for the physical
frame number, and one bit to mark invalid pages. There is a spare
bit which we could use. Here are some possibilities.

It could mark read-only pages. We could use this to protect pages
with code on them and page fault if there's a write to them. I
did think of using this for copy-on-write pages, except that the
write to a page happens *before* the page fault. By then, it's too
late to copy the unaltered page and let the write occur. Damn!

If we can read the page table entries, we could use this bit to
identify dirty pages: ones that have been written to. If we ever
get into paging frames to/from the block storage, this would show
us which frames need to be written out and which frames are clean
and don't need to be written out. Examples of clean pages are pages
of code that come from the executable file for the program.

## Using Another CPU

I think the design would lend itself to use by other 8-bit CPUs:
6502, Z80 etc. For most of them it would be difficult to work
out when to switch between user/kernel modes. But the CPLD gives
the design flexibility and designers can try things out without
a big hardware redesign.
