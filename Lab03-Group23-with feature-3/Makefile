CC = gcc
CFLAGS = -Wall -Wextra -g

all: shell.out

shell.out: shell.c parser.c parser.h
	$(CC) $(CFLAGS) -o shell.out shell.c parser.c

test: test.c parser.c parser.h
	$(CC) $(CFLAGS) -o test.out test.c parser.c
	./test.out

clean:
	rm -f shell.out test.out

.PHONY: all test clean
