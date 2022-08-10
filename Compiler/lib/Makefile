z: z.o libc.a
	lwlink -f srec --section-base=text=0x100 \
		--section-base=registers=0xc0 \
		-L /home/wkt/wktcloud/MMU09/Compiler/lib \
		-o z z.o -lc

libc.a: crt0.o
	lwar -c libc.a crt0.o

crt0.o: crt0.s
	lwasm -f obj -o crt0.o crt0.s

z.o: z.s
	lwasm -f obj -o z.o z.s

clean:
	rm -rf z *.o libc.a
