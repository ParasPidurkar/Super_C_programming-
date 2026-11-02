CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
LDLIBS  := -lrt
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer

# Self-contained examples, run in order by "make run".
DEMOS   := 01_posix_basic 02_posix_priority 03_posix_nonblock 04_posix_notify \
           05_sysv_msgqueue
# Interactive pair, run by hand in two terminals.
CHAT    := mq_producer mq_consumer

all: $(DEMOS) $(CHAT)

%: %.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

run: $(DEMOS)
	@for p in $(DEMOS); do echo "=== $$p ==="; ./$$p || exit 1; echo; done

check:
	@for p in $(DEMOS); do \
		$(CC) $(CFLAGS) $(SAN) -o $$p-san $$p.c $(LDLIBS) || exit 1; \
		echo "=== $$p (ASan+UBSan) ==="; ./$$p-san || exit 1; echo; \
	done

# Remove POSIX queues left behind by a crashed or interrupted run.
unlink:
	rm -f /dev/mqueue/learn_mq_*

clean:
	rm -f $(DEMOS) $(CHAT) *-san

.PHONY: all run check unlink clean
