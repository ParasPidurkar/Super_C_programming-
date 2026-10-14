CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer

test: test.c aqueue.c aqueue.h lqueue.h
	$(CC) $(CFLAGS) -o $@ test.c aqueue.c

demo: demo.c aqueue.c aqueue.h lqueue.h
	$(CC) $(CFLAGS) -o $@ demo.c aqueue.c

check: test.c aqueue.c aqueue.h lqueue.h
	$(CC) $(CFLAGS) $(SAN) -o test-san test.c aqueue.c
	./test-san

clean:
	rm -f test test-san demo

.PHONY: check clean
