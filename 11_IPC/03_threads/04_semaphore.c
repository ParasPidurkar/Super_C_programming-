// SPDX-License-Identifier: GPL-2.0
/*
 * 04_semaphore - limit how many threads do something at once.
 *
 * WHAT IS A SEMAPHORE?
 * A counter that never goes below zero, with two operations:
 *
 *   sem_wait(&s)   "take one": if s > 0, decrement and continue;
 *                  if s == 0, SLEEP until someone posts, then decrement.
 *   sem_post(&s)   "give one back": increment, waking one sleeper.
 *
 * Think of it as a bowl of N tokens. To enter you must take a token;
 * when you leave you put it back. With 2 tokens, at most 2 threads are
 * inside; the third waits until a token comes back.
 *
 * TWO CLASSIC USES (both shown below)
 *   init to N  ->  "at most N at a time" (connection pool, rate limit)
 *   init to 0  ->  "event happened": one thread posts, another waits.
 *                  Unlike pthread_cond_signal, a post IS remembered:
 *                  if nobody is waiting yet, the count stays raised.
 *
 * SEMAPHORE vs MUTEX
 *   - a mutex has an owner: only the thread that locked may unlock.
 *   - a semaphore has no owner: any thread may post.
 *   - a semaphore initialised to 1 behaves like a mutex, but use a real
 *     mutex for that; it can detect misuse and supports priority
 *     inheritance.
 */
#define _POSIX_C_SOURCE 200809L

#include <semaphore.h>	/* sem_t, sem_init, sem_wait, sem_post */
#include <stdatomic.h>

#include "common.h"

#define WORKERS 6
#define SLOTS 2		/* at most this many workers inside at once */

static sem_t slots;	/* counts free slots: starts at SLOTS */
static sem_t done;	/* counts finished workers: starts at 0 */

/*
 * Statistics, just to PROVE the limit works. They're shared between
 * workers, so they're atomics (see example 02).
 */
static atomic_int inside, max_inside;

static void *worker(void *p)
{
	int id = *(int *)p, n, m;

	/*
	 * Take a slot. The first SLOTS workers get one straight away; the
	 * rest sleep here until a worker inside calls sem_post(&slots).
	 */
	sem_wait(&slots);

	/* ---- at most SLOTS threads can be in this part at a time ---- */

	/* Count ourselves in. fetch_add returns the OLD value, so +1. */
	n = atomic_fetch_add(&inside, 1) + 1;

	/*
	 * Record the highest number ever seen inside: "max_inside =
	 * max(max_inside, n)" done atomically. compare_exchange stores n
	 * only if max_inside still equals m; if another thread changed it
	 * first, it reloads m and we try again.
	 */
	m = atomic_load(&max_inside);
	while (n > m && !atomic_compare_exchange_weak(&max_inside, &m, n))
		;

	printf("  worker %d in  (%d inside)\n", id, n);
	msleep(30);	/* the "limited" work: e.g. using a DB connection */
	atomic_fetch_sub(&inside, 1);
	printf("  worker %d out\n", id);

	/* Give the slot back. This wakes one worker sleeping in sem_wait. */
	sem_post(&slots);

	/* ---- end of limited section ---- */

	/* Second use: tell main "one more worker finished". */
	sem_post(&done);
	return NULL;
}

int main(void)
{
	pthread_t tid[WORKERS];
	int ids[WORKERS], i;

	/*
	 * sem_init(&sem, pshared, initial_value)
	 *   pshared = 0: shared only between threads of THIS process.
	 *   pshared = 1: shared between processes too; the sem_t must then
	 *                live in shared memory (mmap / shm_open).
	 * For unrelated processes use a named semaphore: sem_open("/name").
	 *
	 * Unlike pthread functions, sem_*() DO set errno, so we use die().
	 */
	if (sem_init(&slots, 0, SLOTS) || sem_init(&done, 0, 0))
		die("sem_init");

	printf("%d workers, %d slots\n", WORKERS, SLOTS);
	for (i = 0; i < WORKERS; i++) {
		ids[i] = i;
		check(pthread_create(&tid[i], NULL, worker, &ids[i]),
		      "pthread_create");
		/*
		 * Detach right away: we don't need their return values. A
		 * detached thread can't be joined, so we need another way
		 * to know when they are finished; that's the "done"
		 * semaphore.
		 */
		check(pthread_detach(tid[i]), "pthread_detach");
	}

	/*
	 * Wait once per worker. "done" starts at 0, so each sem_wait
	 * sleeps until some worker has posted. If a worker already posted,
	 * the count is waiting for us and sem_wait returns at once.
	 *
	 * sem_wait() can fail with EINTR if a signal handler runs while we
	 * sleep; in that case just wait again.
	 */
	for (i = 0; i < WORKERS; i++)
		while (sem_wait(&done) && errno == EINTR)
			;

	printf("  never more than %d inside at once (limit %d)\n",
	       atomic_load(&max_inside), SLOTS);

	/* Destroy a semaphore only once no thread can still be using it. */
	sem_destroy(&slots);
	sem_destroy(&done);
	return atomic_load(&max_inside) <= SLOTS ? EXIT_SUCCESS : EXIT_FAILURE;
}
