CC = gcc
CFLAGS = -g -Wall

all: oriaz

run: 
	./oriaz

v:
	valgrind ./oriaz

oriaz: history.o oriaz.o
	$(CC) $(CFLAGS) -o oriaz oriaz.o history.o

oriaz.o: oriaz.c myshell.h history.h
	$(CC) $(CFLAGS) -c oriaz.c

history.o: history.c history.h
	$(CC) $(CFLAGS) -c history.c

.PHONY: all clean

clean:
	rm -f *.o oriaz mylog log