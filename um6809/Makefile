CFLAGS := -Wall -g

default: um6809

6809.o: 6809.c
	$(CC) $(CFLAGS) -c 6809.c

monitor.o: monitor.c
	$(CC) $(CFLAGS) -c monitor.c

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

oscode.o: oscode.c
	$(CC) $(CFLAGS) -c oscode.c

um6809: 6809.o monitor.o main.o oscode.o
	$(CC) $(CFLAGS) -o um6809 main.o monitor.o 6809.o oscode.o -lreadline

clean:
	rm -f um6809 *.o
