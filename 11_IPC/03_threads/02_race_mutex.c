// SPDX-License-Identifier: GPL-2.0
/*
 * 02_race_mutex - a data race, and two ways to fix it.
 *
 * WHAT IS A DATA RACE?
 * Two threads access the same variable at the same time, at least one of
 * them writes, and nothing makes them take turns.
 *
 * "counter++" looks like one step, but the CPU does three:
 *
 *     load  register <- counter
 *     add   register <- register + 1
 *     store counter  <- register
 *
 * Two threads can interleave those steps and lose an increment:
 *
 *     thread A            thread B            counter
 *     load  (5)                                  5
 *                         load  (5)              5
 *     add   -> 6                                 5
 *                         add   -> 6             5
 *     store 6                                    6
 *                         store 6                6   <- should be 7!
 *
 * FIX 1: MUTEX (MUTual EXclusion lock)
 *   lock();  counter++;  unlock();
 *   Only one thread at a time can hold the lock. Any other thread that
 *   calls lock() sleeps until it is released. The code between lock and
 *   unlock is called the "critical section".
 *
 * FIX 2: ATOMIC OPERATION (C11 <stdatomic.h>)
 *   atomic_fetch_add(&counter, 1);
 *   The CPU does load+add+store as ONE indivisible instruction (on x86,
 *   "lock xadd"). No lock, no sleeping, so it's cheaper, but it only
 *   protects a single variable.
 *
 * The unsafe version below is undefined behaviour on purpose, to show
 * the bug. Never write it in real code.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdatomic.h>

#include "common.h"

#define NTHREADS 4
#define LOOPS 1000000L

/*
 * Three counters, one per technique. All are globals, so every thread
 * sees the SAME variable; that sharing is what causes the problem.
 *
 * "volatile" here only stops the optimiser from turning the loop into
 * one "racy += LOOPS", which would hide the bug. volatile does NOT make
 * anything thread-safe. It is not a synchronisation tool.
 */
static volatile long racy;

static long locked;
/*
 * PTHREAD_MUTEX_INITIALIZER sets up a mutex declared as a static or
 * global. For one created at run time (e.g. inside a malloc'd struct),
 * call pthread_mutex_init() and later pthread_mutex_destroy().
 */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* atomic_long: every operation on it is indivisible. Starts at 0. */
static atomic_long atomic_counter;

/* WRONG: no protection at all, so increments get lost. */
static void *inc_racy(void *p)
{
	long i;

	(void)p;
	for (i = 0; i < LOOPS; i++)
		racy++;		/* load / add / store; threads collide here */
	return NULL;
}

/* RIGHT: a mutex makes the increment one step for other threads. */
static void *inc_locked(void *p)
{
	long i;

	(void)p;
	for (i = 0; i < LOOPS; i++) {
		/*
		 * If another thread holds the lock, this call SLEEPS until
		 * it is unlocked. The kernel then wakes one waiting thread.
		 */
		pthread_mutex_lock(&lock);

		locked++;	/* critical section: one thread at a time */

		/*
		 * Release it. Unlock also acts as a "memory barrier": our
		 * write to locked is guaranteed visible to the next thread
		 * that locks. Always unlock from the thread that locked.
		 */
		pthread_mutex_unlock(&lock);
	}
	/*
	 * Note: locking 1,000,000 times is slow (see the timings). In real
	 * code you'd hold the lock for a bigger chunk of work, or better,
	 * count locally and add the total once at the end.
	 */
	return NULL;
}

/* RIGHT and cheaper: one atomic CPU instruction per increment. */
static void *inc_atomic(void *p)
{
	long i;

	(void)p;
	for (i = 0; i < LOOPS; i++)
		atomic_fetch_add(&atomic_counter, 1);	/* indivisible ++ */
	return NULL;
}

/*
 * Start NTHREADS threads running @fn, wait for them, then compare the
 * counter (read through @get) with the value it should have.
 */
static void run(const char *name, void *(*fn)(void *), long (*get)(void))
{
	pthread_t tid[NTHREADS];
	double t0 = now();
	long got;
	int i;

	for (i = 0; i < NTHREADS; i++)
		check(pthread_create(&tid[i], NULL, fn, NULL), "pthread_create");
	for (i = 0; i < NTHREADS; i++)
		check(pthread_join(tid[i], NULL), "pthread_join");

	/* All threads have finished, so reading the counter is safe now. */
	got = get();
	printf("  %-7s %8ld / %ld  lost %7ld  %6.1f ms\n", name, got,
	       NTHREADS * LOOPS, NTHREADS * LOOPS - got, (now() - t0) * 1e3);
}

/* Small readers so run() can print any of the three counters. */
static long get_racy(void) { return racy; }
static long get_locked(void) { return locked; }
static long get_atomic(void) { return atomic_load(&atomic_counter); }

int main(void)
{
	printf("%d threads each increment a shared counter %ld times\n",
	       NTHREADS, LOOPS);
	run("racy", inc_racy, get_racy);
	run("mutex", inc_locked, get_locked);
	run("atomic", inc_atomic, get_atomic);

	/*
	 * Rule of thumb:
	 *   - one counter or flag             -> atomic
	 *   - several variables that must
	 *     change together (e.g. a list)   -> mutex
	 *   - best of all: don't share. Give each thread its own data and
	 *     combine the results after join (like example 01 does).
	 */
	puts("racy loses updates (the exact number changes every run);");
	puts("mutex and atomic are always exact; atomic is cheaper.");
	return EXIT_SUCCESS;
}
