ASM=vasm6809_std
ASMFLAGS=-quiet -nowarn=62 -opt-branch -opt-offset -Fvobj

.s.o:
	$(ASM) $(ASMFLAGS) $< -o $@

startup.o: startup.s

clean:
	rm -f startup.o
