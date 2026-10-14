CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer

test: test.c bst.c bst.h
	$(CC) $(CFLAGS) -o $@ test.c bst.c

demo: demo.c bst.c bst.h
	$(CC) $(CFLAGS) -o $@ demo.c bst.c

check: test.c bst.c bst.h
	$(CC) $(CFLAGS) $(SAN) -o test-san test.c bst.c
	./test-san

clean:
	rm -f test test-san demo

.PHONY: check clean
