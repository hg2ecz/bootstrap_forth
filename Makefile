.POSIX:

CC=gcc
CFLAGS=-O2 -Wall
LDFLAGS=-s

forth: forth.c words.h
	$(CC) $(CFLAGS) $(LDFLAGS) forth.c -o $@

words.h: forth.c
	sed -ne '/^W/s/) .*/)/p' $< >$@ || rm $@

clean:
	rm -f forth words.h
