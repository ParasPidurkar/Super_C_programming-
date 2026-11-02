// SPDX-License-Identifier: GPL-2.0
/*
 * 05_tls_once - per-thread data and one-time initialisation.
 *
 * THREAD-LOCAL STORAGE (TLS)
 * A normal global has ONE copy that every thread shares. Sometimes each
 * thread needs its OWN copy of a global-like variable instead. The best
 * known example is errno: if it were shared, thread A's failed call
 * could overwrite the error code thread B is about to read.
 *
 * Two ways to get per-thread variables:
 *
 *   1. _Thread_local int x;            (C11 keyword; "__thread" in GCC)
 *      Simplest. Each thread sees its own x, starting at its initial
 *      value. Fine for plain values.
 *
 *   2. pthread_key_create(&key, destructor)
 *      Creates a "slot" that holds one void * per thread:
 *          pthread_setspecific(key, ptr)   store this thread's pointer
 *          pthread_getspecific(key)        fetch it (NULL if never set)
 *      When a thread exits, the destructor is called with its pointer,
 *      so per-thread malloc'd buffers get freed automatically.
 *
 * ONE-TIME INITIALISATION
 * pthread_once(&once, fn) calls fn() exactly once in the whole program,
 * even if many threads call pthread_once() at the same moment. Threads
 * that arrive while fn() is running wait until it is done. Use it for
 * lazy setup of shared things (here, creating the key).
 */
#define _POSIX_C_SOURCE 200809L

#include "common.h"

#define NTHREADS 3

/*
 * One instance per thread, each starting at 0. Threads change their
 * own copy freely, with no lock, because nobody else can see it.
 */
static _Thread_local int my_count;

/* pthread_once needs a control variable initialised like this. */
static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_key_t buf_key;	/* the per-thread slot, created in init() */

/*
 * Destructor for buf_key. pthreads calls it automatically when a thread
 * exits, if that thread's value is not NULL. It runs IN the exiting
 * thread.
 */
static void free_buf(void *p)
{
	printf("  destructor: freeing \"%s\"\n", (char *)p);
	free(p);
}

/* Run once, by whichever thread calls pthread_once() first. */
static void init(void)
{
	puts("  init: runs exactly once");
	check(pthread_key_create(&buf_key, free_buf), "pthread_key_create");
}

/*
 * Return this thread's private buffer, allocating it on first use.
 * This is the typical pattern for a thread-safe library function that
 * needs a scratch buffer (think strerror() or localtime()).
 */
static char *thread_buf(void)
{
	char *b;

	/* Make sure the key exists. Cheap after the first call. */
	pthread_once(&once, init);

	/* Each thread gets ITS OWN value back; NULL the first time. */
	b = pthread_getspecific(buf_key);
	if (!b) {
		b = malloc(32);
		if (!b)
			die("malloc");
		/* Store it. Only this thread will ever see this pointer. */
		check(pthread_setspecific(buf_key, b), "pthread_setspecific");
	}
	return b;
}

static void *work(void *p)
{
	int id = *(int *)p, i;
	char *b;

	/*
	 * Thread 0 adds 1, thread 1 adds 2, thread 2 adds 3. Each starts
	 * from 0 because each has its own my_count, so they never add up
	 * across threads, and main's copy stays 0.
	 */
	for (i = 0; i <= id; i++)
		my_count++;

	b = thread_buf();
	snprintf(b, 32, "buffer of thread %d", id);
	printf("  thread %d: my_count = %d, buf = \"%s\"\n", id, my_count, b);

	/* No free(b) here: the key's destructor frees it when we return. */
	return NULL;
}

int main(void)
{
	pthread_t tid[NTHREADS];
	int ids[NTHREADS], i;

	/*
	 * Threads run one after another here (create, then join right
	 * away) so the output is easy to follow. Thread-local storage
	 * works the same when they run at the same time.
	 */
	for (i = 0; i < NTHREADS; i++) {
		ids[i] = i;
		check(pthread_create(&tid[i], NULL, work, &ids[i]),
		      "pthread_create");
		check(pthread_join(tid[i], NULL), "pthread_join");
	}

	/* main is a thread too, with its own my_count, never incremented. */
	printf("  main: my_count = %d (its own copy, untouched)\n", my_count);

	/* Delete the key when no thread uses it any more. */
	pthread_key_delete(buf_key);
	return EXIT_SUCCESS;
}
