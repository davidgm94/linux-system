# HEADERS = main.h headers.h

CC=/usr/bin/clang-10
CFLAGS=-g -O2
LIBS=-lpthread -lgcc
LINKER_FLAGS=$(LIBS)
HEADERS=common.h

default: main

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

main: main.o
	$(CC) main.o -o main -g $(LINKER_FLAGS)

clean:
	-rm -f main.o
	-rm -f main
