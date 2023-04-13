CC = gcc
CFLAGS = -g -Wall

all: myshell

run: 
	./myshell

myshell: history.o myshell.o
	$(CC) $(CFLAGS) -o myshell myshell.o history.o

myshell.o: myshell.c myshell.h history.h
	$(CC) $(CFLAGS) -c myshell.c

history.o: history.c history.h
	$(CC) $(CFLAGS) -c history.c

.PHONY: all clean

clean:
	rm -f *.o myshell