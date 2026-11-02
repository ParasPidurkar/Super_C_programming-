CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
CFLAGS  += -pthread
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer

DEMOS   := c11_threads

all: $(DEMOS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

run: $(DEMOS)
	@for p in $(DEMOS); do echo "=== $$p ==="; ./$$p || exit 1; echo; done

check:
	@for p in $(DEMOS); do \
		$(CC) $(CFLAGS) $(SAN) -o $$p-san $$p.c || exit 1; \
		echo "=== $$p (ASan+UBSan) ==="; ./$$p-san || exit 1; echo; \
	done

# No "tsan" target: ThreadSanitizer (gcc 13 and clang 18) cannot see
# threads started by glibc's thrd_create(), which calls an internal
# pthread_create TSan does not intercept, and any thrd_create crashes
# with a SEGV at startup. Use ../03_threads (pthreads) to try TSan.

clean:
	rm -f $(DEMOS) *-san *-tsan

.PHONY: all run check clean
