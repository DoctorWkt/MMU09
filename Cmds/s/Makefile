CC= vc '+mmu09'
CFLAGS= -O2

s:	s.o address.o adjust.o Bman.o buffer.o commands.o \
	keyboard.o lib.o operator.o screen.o Sman.o yank.o
	$(CC) -o s *.o
	cp s ../../Build/bin/vi

clean:
	rm -f *.o s bla
