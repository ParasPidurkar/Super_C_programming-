CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer

# Self-contained examples, run in order by "make run".
DEMOS   := 01_default_actions 02_handlers 03_masking 04_sigchld 05_signalfd
# Interactive program, run by hand.
SERVER  := server

all: $(DEMOS) $(SERVER)

%: %.c common.h
	$(CC) $(CFLAGS) -o $@ $<

run: $(DEMOS)
	@for p in $(DEMOS); do echo "=== $$p ==="; ./$$p || exit 1; echo; done

check:
	@for p in $(DEMOS); do \
		$(CC) $(CFLAGS) $(SAN) -o $$p-san $$p.c || exit 1; \
		echo "=== $$p (ASan+UBSan) ==="; ./$$p-san || exit 1; echo; \
	done

clean:
	rm -f $(DEMOS) $(SERVER) *-san

.PHONY: all run check clean
