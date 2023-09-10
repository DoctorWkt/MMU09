CFLAGS := -Wall -g

OBJS= 6809.o memory.o monitor.o main.o ch375.o uart.o

6809: $(OBJS)
	$(CC) $(CFLAGS) -o 6809 $(OBJS) -lreadline

clean:
	rm -f 6809 *.o debug.out
