# 6809 stuff
BINFMT=srec19
ASM=vasm6809_std
ASMFLAGS=-quiet -nowarn=62 -opt-branch -opt-offset -Fvobj
CC= vc '+mmu09'
CFLAGS= -O2
KERNOBJS= romfuncs.o blk.o bio.o file.o fs.o sysfile.o \
	proc.o pipe.o cprintf.o \
	memset.o strncpy.o strncmp.o lsl.o asrl.o div.o

.c.o:
	$(CC) -c $(CFLAGS) $<

.s.o:
	$(ASM) $(ASMFLAGS) $< -o $@
	
all: xv6rom

xv6rom: $(KERNOBJS) romfuncs.s
	vlink -b $(BINFMT) -Tvlink.cmd $(KERNOBJS) -o xv6rom -Mmap

lsl.o:
	co lsl.o

asrl.o:
	co asrl.o

div.o:
	co div.o

xv6rom.img: xv6rom
	srec_cat xv6rom -Motorola -output temp -Binary 2> /dev/null
	dd if=temp bs=256 count=127 of=xv6rom.img
	dd if=temp bs=256 skip=255 count=1 >> xv6rom.img
	rm -f temp

sfiles: blk.c bio.c file.c fs.c sysfile.c
	cfm -S blk.c
	cfm -S bio.c
	cfm -S file.c
	cfm -S fs.c
	cfm -S sysfile.c

clean:
	rm -f *.a *.o *mkfs *.img *.lst *.map *.link \
	xv6rom.s blk.s bio.s file.s fs.s sysfile.s strncmp.s vectors.s19 \
	xv6rom xv6rom2 lxv6rom libxv6fs.a ls mkdir catinto cat usertests \
	xv6rom.img Z/_* romcalls.s map bla
