/* SPDX-License-Identifier: GPL-2.0 */
/*
 * common.h - small helpers shared by the thread examples.
 *
 * Every .c file defines _POSIX_C_SOURCE before including anything.
 * With -std=c11 the compiler hides everything that is not plain ISO C;
 * that macro says "I also want POSIX 2008", which makes pthreads,
 * semaphores, nanosleep() and clock_gettime() visible again.
 *
 * Build every file with -pthread. It does two jobs: links the pthread
 * library, and defines macros (like _REENTRANT) that make the C library
 * thread-safe. Using only -lpthread does the first job, not the second.
 */
#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <pthread.h>	/* pthread_create, mutexes, condition variables... */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 * Print what failed and why, then exit. For ordinary system calls
 * (malloc, sem_init, ...) that report errors through errno.
 */
_Noreturn static inline void die(const char *what)
{
	perror(what);	/* prints "what: <text for errno>" */
	exit(EXIT_FAILURE);
}

/*
 * Error check for pthread_*() functions.
 *
 * IMPORTANT DIFFERENCE: pthread functions do NOT set errno. They return
 * 0 on success or the error number itself (EINVAL, EAGAIN, ...) on
 * failure. So perror() would print the wrong message; we pass the
 * returned number to strerror() instead.
 *
 * Usage:  check(pthread_create(...), "pthread_create");
 */
static inline void check(int err, const char *what)
{
	if (err) {
		fprintf(stderr, "%s: %s\n", what, strerror(err));
		exit(EXIT_FAILURE);
	}
}

/*
 * Sleep @ms milliseconds. In the examples this stands in for "doing
 * some work", and makes threads overlap so you can see them interleave.
 *
 * nanosleep() stops early with EINTR if a signal arrives; it then
 * writes the remaining time back into ts, so we just call it again.
 */
static inline void msleep(long ms)
{
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };

	while (nanosleep(&ts, &ts) && errno == EINTR)
		;
}

/*
 * Current time in seconds, for measuring how long something took.
 * CLOCK_MONOTONIC never jumps backwards (unlike wall-clock time, which
 * NTP or the user can change), so it is the right clock for timing.
 * Only the difference between two calls is meaningful.
 */
static inline double now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

#endif /* COMMON_H */
