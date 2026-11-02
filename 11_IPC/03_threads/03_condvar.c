// SPDX-License-Identifier: GPL-2.0
/*
 * 03_condvar - producer/consumer over a bounded buffer.
 *
 * THE PROBLEM
 * Producers put items into a buffer of fixed size, and consumers take
 * them out. A consumer that finds the buffer empty must wait, and so
 * must a producer that finds it full. How should they wait?
 *
 *   Bad:  while (count == 0) { unlock; lock; }   spins, burns 100% CPU
 *   Good: a CONDITION VARIABLE: sleep until another thread says
 *         "something changed, check again"
 *
 * HOW A CONDITION VARIABLE WORKS
 * It is always used together with a mutex that protects the data:
 *
 *   pthread_mutex_lock(&lock);
 *   while (!condition)                    <- check the data under the lock
 *           pthread_cond_wait(&cv, &lock);
 *   ...use the data...
 *   pthread_mutex_unlock(&lock);
 *
 * pthread_cond_wait() does three things:
 *   1. unlocks the mutex (so other threads can change the data),
 *   2. sleeps until signalled,
 *   3. LOCKS the mutex again before returning.
 * Steps 1 and 2 happen atomically, so a signal can't slip in between
 * them and get missed.
 *
 * The other side changes the data, then wakes a waiter:
 *   pthread_cond_signal(&cv);     wake ONE waiting thread
 *   pthread_cond_broadcast(&cv);  wake ALL waiting threads
 *
 * WHY "while" AND NOT "if"
 *   - Spurious wakeups: POSIX allows wait() to return without a signal.
 *   - Stolen wakeups: between the signal and our re-lock, another
 *     consumer may have grabbed the item. So re-check the condition
 *     every time we wake up.
 */
#define _POSIX_C_SOURCE 200809L

#include "common.h"

#define CAP 4		/* buffer slots; small, so producers do block */
#define PRODUCERS 2
#define CONSUMERS 3
#define ITEMS 10	/* items made by each producer */
#define STOP (-1)	/* "poison pill": tells a consumer to exit */

/*
 * The shared buffer is a circular queue (ring buffer):
 *   head  = index of the next item to take
 *   tail  = index of the next free slot to fill
 *   count = items currently stored
 * Indexes wrap around with "% CAP".
 *
 *   [ . | A | B | . ]     head=1 tail=3 count=2
 *
 * ALL of these are protected by "lock". No thread may touch them
 * without holding it.
 */
static int buf[CAP];
static int head, tail, count;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Two condition variables, one per thing we wait for:
 *   not_empty: consumers sleep on it when count == 0
 *   not_full:  producers sleep on it when count == CAP
 * You could use one for both, but then signal() might wake the wrong
 * kind of thread.
 */
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

/* Add @v to the buffer, sleeping while it is full. */
static void put(int v)
{
	pthread_mutex_lock(&lock);

	/* Full: release the lock and sleep until a consumer makes room. */
	while (count == CAP)
		pthread_cond_wait(&not_full, &lock);
	/* Here we hold the lock again AND count < CAP is guaranteed. */

	buf[tail] = v;
	tail = (tail + 1) % CAP;
	count++;

	/*
	 * The buffer now has at least one item, so wake one consumer that
	 * may be sleeping on not_empty. If none is waiting, the signal is
	 * simply ignored (it is not remembered, unlike a semaphore).
	 */
	pthread_cond_signal(&not_empty);
	pthread_mutex_unlock(&lock);
}

/* Remove and return one item, sleeping while the buffer is empty. */
static int get(void)
{
	int v;

	pthread_mutex_lock(&lock);

	/* Empty: release the lock and sleep until a producer adds one. */
	while (count == 0)
		pthread_cond_wait(&not_empty, &lock);

	v = buf[head];
	head = (head + 1) % CAP;
	count--;

	/* A slot is free now, so wake one producer that may be waiting. */
	pthread_cond_signal(&not_full);
	pthread_mutex_unlock(&lock);
	return v;
}

/* Each producer makes ITEMS values: producer 1 -> 101..110, 2 -> 201.. */
static void *producer(void *p)
{
	int id = *(int *)p, i;

	for (i = 1; i <= ITEMS; i++) {
		put(id * 100 + i);
		msleep(5);	/* pretend producing takes time */
	}
	return NULL;
}

/*
 * Consumers loop until they receive the STOP value. Each one adds up
 * what it got into its own slot in main's sums[] array: no sharing, so
 * no lock is needed for the sum.
 */
static void *consumer(void *p)
{
	long *sum = p;
	int v, n = 0;

	while ((v = get()) != STOP) {
		*sum += v;
		n++;
	}
	printf("  consumer got %2d items, sum %ld\n", n, *sum);
	return NULL;
}

int main(void)
{
	pthread_t prod[PRODUCERS], cons[CONSUMERS];
	int ids[PRODUCERS];
	long sums[CONSUMERS] = { 0 }, total = 0, expect = 0;
	int i, j;

	printf("%d producers x %d items -> buffer[%d] -> %d consumers\n",
	       PRODUCERS, ITEMS, CAP, CONSUMERS);

	/* Start consumers first: they immediately block on the empty buffer. */
	for (i = 0; i < CONSUMERS; i++)
		check(pthread_create(&cons[i], NULL, consumer, &sums[i]),
		      "pthread_create");
	for (i = 0; i < PRODUCERS; i++) {
		ids[i] = i + 1;	/* own int per thread, not &i */
		check(pthread_create(&prod[i], NULL, producer, &ids[i]),
		      "pthread_create");
	}

	/*
	 * HOW TO STOP THE CONSUMERS
	 * They are blocked inside get(), so they can't check a flag. Once
	 * all producers are done, send one STOP item per consumer. Each
	 * consumer takes exactly one pill and exits. Because the buffer is
	 * a FIFO, every real item is taken before any pill.
	 */
	for (i = 0; i < PRODUCERS; i++)
		check(pthread_join(prod[i], NULL), "pthread_join");
	for (i = 0; i < CONSUMERS; i++)
		put(STOP);
	for (i = 0; i < CONSUMERS; i++) {
		check(pthread_join(cons[i], NULL), "pthread_join");
		total += sums[i];
	}

	/* Prove no item was lost or delivered twice. */
	for (i = 1; i <= PRODUCERS; i++)
		for (j = 1; j <= ITEMS; j++)
			expect += i * 100 + j;
	printf("  total %ld, expected %ld: %s\n", total, expect,
	       total == expect ? "every item delivered exactly once" : "BUG");
	return total == expect ? EXIT_SUCCESS : EXIT_FAILURE;
}
