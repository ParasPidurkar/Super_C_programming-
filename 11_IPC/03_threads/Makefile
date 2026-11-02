CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
CFLAGS  += -pthread
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer
TSAN    := -fsanitize=thread

# Self-contained examples, run in order by "make run".
DEMOS   := 01_create_join 02_race_mutex 03_condvar 04_semaphore 05_tls_once \
           06_thread_signals
# 02 races on purpose, so ThreadSanitizer would (correctly) fail it.
# setarch -R disables ASLR: TSan crashes on kernels with high mmap_rnd_bits.
RACE_FREE := $(filter-out 02_race_mutex,$(DEMOS))

all: $(DEMOS)

%: %.c common.h
	$(CC) $(CFLAGS) -o $@ $<

run: $(DEMOS)
	@for p in $(DEMOS); do echo "=== $$p ==="; ./$$p || exit 1; echo; done

check:
	@for p in $(DEMOS); do \
		$(CC) $(CFLAGS) $(SAN) -o $$p-san $$p.c || exit 1; \
		echo "=== $$p (ASan+UBSan) ==="; ./$$p-san || exit 1; echo; \
	done

tsan:
	@for p in $(RACE_FREE); do \
		$(CC) $(CFLAGS) $(TSAN) -o $$p-tsan $$p.c || exit 1; \
		echo "=== $$p (TSan) ==="; setarch $$(uname -m) -R ./$$p-tsan || exit 1; echo; \
	done

clean:
	rm -f $(DEMOS) *-san *-tsan

.PHONY: all run check tsan clean
