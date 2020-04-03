# HEADERS = main.h headers.h

CC=/usr/bin/clang-9
CFLAGS=-g -O3
LIBS=-pthread
HEADERS= common.h
default: main

main.o: main.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

main: main.o
	$(CC) main.o -o main $(CFLAGS) $(LIBS)

clean:
	-rm -f main.o
	-rm -f main
