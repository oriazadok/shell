CC = gcc
CFLAGS = -g -Wall

all: myshell

run: 
	./myshell

myshell: myshell.c
	$(CC) $(CFLAGS) -o myshell myshell.c

.PHONY: all clean

clean:
	rm -f *.o myshell mylog log