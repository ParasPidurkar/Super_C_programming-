/* SPDX-License-Identifier: GPL-2.0 */
/*
 * common.h - small helpers shared by the signal examples.
 *
 * Every .c file defines _XOPEN_SOURCE (or _GNU_SOURCE) before including
 * anything, so sigaction() and friends are visible under -std=c11.
 */
#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Print what failed and why (from errno), then exit. */
_Noreturn static inline void die(const char *what)
{
	perror(what);
	exit(EXIT_FAILURE);
}

/*
 * Print from inside a signal handler. printf() is NOT async-signal-safe
 * (it takes locks and may allocate); write() and strlen() are.
 */
static inline void say(const char *s)
{
	if (write(STDOUT_FILENO, s, strlen(s)) < 0) {
		/* nothing useful to do inside a handler */
	}
}

/* Install @fn for @sig with sigaction(). @flags: SA_RESTART etc. */
static inline void install(int sig, void (*fn)(int), int flags)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = fn;
	sigemptyset(&sa.sa_mask);	/* block nothing extra while running */
	sa.sa_flags = flags;
	if (sigaction(sig, &sa, NULL))
		die("sigaction");
}

/* Sleep @ms milliseconds, resuming after any signal handler runs. */
static inline void msleep(long ms)
{
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };

	while (nanosleep(&ts, &ts) && errno == EINTR)
		;
}

#endif /* COMMON_H */
