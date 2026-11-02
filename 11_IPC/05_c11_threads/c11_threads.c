// SPDX-License-Identifier: GPL-2.0
/*
 * c11_threads - the C11 standard thread library, <threads.h>, in one file.
 *
 * WHAT IS <threads.h>?
 * Since C11 (2011) threads are part of the C language standard itself,
 * not just of POSIX. Any C11 compiler + C library that supports them
 * gives you the same API on Linux, Windows (MSVC 2022+), the BSDs...
 * On Linux, glibc 2.28+ implements it as a thin layer over pthreads,
 * so the behaviour and the cost are exactly the same.
 *
 * If the C library does NOT support it, it defines __STDC_NO_THREADS__.
 *
 * THE WHOLE API (it is small):
 *
 *   threads          thrd_t, thrd_create, thrd_join, thrd_detach,
 *                    thrd_exit, thrd_current, thrd_equal, thrd_sleep,
 *                    thrd_yield
 *   mutexes          mtx_t, mtx_init, mtx_lock, mtx_trylock,
 *                    mtx_timedlock, mtx_unlock, mtx_destroy
 *   condition vars   cnd_t, cnd_init, cnd_wait, cnd_timedwait,
 *                    cnd_signal, cnd_broadcast, cnd_destroy
 *   per-thread data  thread_local, tss_t, tss_create, tss_get,
 *                    tss_set, tss_delete
 *   run once         once_flag, ONCE_FLAG_INIT, call_once
 *
 * Every function returns one of:
 *   thrd_success   it worked
 *   thrd_busy      mtx_trylock: someone else holds the lock
 *   thrd_timedout  mtx_timedlock / cnd_timedwait: deadline passed
 *   thrd_nomem     out of memory (thrd_create)
 *   thrd_error     anything else
 *
 * This file is self-contained and uses ONLY standard C11: no
 * _POSIX_C_SOURCE, no <pthread.h>, no <unistd.h>.
 *
 * Parts:
 *   1. create + join, several arguments in a struct, an int result
 *   2. a mutex protecting a shared counter
 *   3. condition variables: a producer/consumer queue
 *   4. thread_local, tss_t (with destructor) and call_once
 *   5. timed locks, and a detached thread we wait for properly
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#ifdef __STDC_NO_THREADS__
#error "this C library has no <threads.h>; use pthreads instead"
#endif

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/*
 * Error check for every thrd_ / mtx_ / cnd_ / tss_ call. Like the
 * pthread functions, these return a status code and do not set errno,
 * so perror() would be useless here.
 */
static void check(int rc, const char *what)
{
	const char *why;

	if (rc == thrd_success)
		return;
	switch (rc) {
	case thrd_nomem:	why = "out of memory";	break;
	case thrd_busy:		why = "busy";		break;
	case thrd_timedout:	why = "timed out";	break;
	default:		why = "error";		break;
	}
	fprintf(stderr, "%s failed: %s\n", what, why);
	exit(EXIT_FAILURE);
}

/*
 * Sleep @ms milliseconds with the standard thrd_sleep().
 * It returns 0 when done, -1 if a signal interrupted it (the remaining
 * time is written back into ts, so we just sleep again), or another
 * negative value on error.
 */
static void msleep(long ms)
{
	struct timespec ts = { .tv_sec = ms / 1000,
			       .tv_nsec = (ms % 1000) * 1000000L };

	while (thrd_sleep(&ts, &ts) == -1)
		;
}

/*
 * The timed functions (mtx_timedlock, cnd_timedwait) take an ABSOLUTE
 * deadline on the TIME_UTC clock ("give up at 10:30:05.250"), not a
 * duration ("give up in 250 ms"). This turns a duration into one.
 * timespec_get() is the standard C11 way to read that clock.
 */
static struct timespec deadline_in(long ms)
{
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);
	ts.tv_sec += ms / 1000;
	ts.tv_nsec += (ms % 1000) * 1000000L;
	if (ts.tv_nsec >= 1000000000L) {	/* carry into seconds */
		ts.tv_sec += 1;
		ts.tv_nsec -= 1000000000L;
	}
	return ts;
}

/* ------------------------------------------------------------------ */
/* Part 1: create + join, several arguments, an int result            */
/* ------------------------------------------------------------------ */

#define NTHREADS 4

/*
 * A C11 thread function has the fixed signature
 *     int function(void *arg)
 * One void * in (same as pthreads), but an INT out (pthreads returns
 * a void *). To pass several values, put them in a struct and pass
 * its address. Bigger results go back through the same struct.
 */
struct job {
	int id;
	long from, to;	/* input: sum the range [from, to] */
	long result;	/* output: written by the thread, read after join */
};

static int sum_range(void *p)
{
	struct job *j = p;	/* void * converts to any object pointer */
	long s = 0;		/* locals live on THIS thread's own stack */

	if (j->from > j->to)
		return -1;	/* the int result is handy as a status code */

	for (long i = j->from; i <= j->to; i++)
		s += i;

	/*
	 * No lock needed: every thread has its OWN struct, and main reads
	 * it only after thrd_join(), which makes this write visible.
	 */
	j->result = s;
	printf("  thread %d: sum(%ld..%ld) = %ld\n", j->id, j->from, j->to, s);
	return 0;	/* same as thrd_exit(0) */
}

static void part1_create_join(void)
{
	thrd_t tid[NTHREADS];
	struct job jobs[NTHREADS];	/* one argument struct PER thread */
	const long per = 1000000 / NTHREADS;
	long total = 0;

	printf("1. split sum(1..1000000) across %d threads\n", NTHREADS);

	for (int i = 0; i < NTHREADS; i++) {
		jobs[i] = (struct job){ .id = i,
					.from = i * per + 1,
					.to = (i + 1) * per };
		/*
		 * thrd_create(&id, function, argument)
		 * No attributes argument: C11 threads are always created
		 * joinable with the default stack size. Never pass &i: the
		 * loop changes i before the thread reads it.
		 */
		check(thrd_create(&tid[i], sum_range, &jobs[i]), "thrd_create");
	}

	for (int i = 0; i < NTHREADS; i++) {
		int status;

		/* thrd_join(id, &result): wait, then fetch the int returned */
		check(thrd_join(tid[i], &status), "thrd_join");
		if (status != 0) {
			fprintf(stderr, "thread %d reported %d\n", i, status);
			exit(EXIT_FAILURE);
		}
		total += jobs[i].result;
	}
	printf("  total = %ld (expected %ld)\n", total, 1000000L * 1000001L / 2);
}

/* ------------------------------------------------------------------ */
/* Part 2: a mutex protecting a shared counter                        */
/* ------------------------------------------------------------------ */

#define INCREMENTS 100000

/*
 * Several threads updating one variable need a lock: "counter++" is
 * really load, add, store, and two threads can interleave those steps
 * and lose updates. ../03_threads/02_race_mutex.c shows the lost
 * updates happening.
 *
 * Unlike PTHREAD_MUTEX_INITIALIZER, C11 has no static initialiser for
 * mtx_t: you must call mtx_init() at run time, and mtx_destroy() when
 * done. The struct keeps the lock next to the data it protects.
 */
struct counter {
	mtx_t lock;
	long value;
};

static int add_many(void *p)
{
	struct counter *c = p;

	for (int i = 0; i < INCREMENTS; i++) {
		check(mtx_lock(&c->lock), "mtx_lock");
		c->value++;	/* only one thread at a time gets here */
		check(mtx_unlock(&c->lock), "mtx_unlock");
	}
	return 0;
}

static void part2_mutex(void)
{
	struct counter c = { .value = 0 };
	thrd_t tid[NTHREADS];

	printf("2. %d threads x %d locked increments\n", NTHREADS, INCREMENTS);

	/*
	 * mtx_init(&m, type). The type is one of:
	 *   mtx_plain                  ordinary lock
	 *   mtx_recursive              the owner may lock it again
	 *   mtx_timed                  also allows mtx_timedlock (part 5)
	 *   mtx_timed | mtx_recursive  both
	 */
	check(mtx_init(&c.lock, mtx_plain), "mtx_init");

	for (int i = 0; i < NTHREADS; i++)
		check(thrd_create(&tid[i], add_many, &c), "thrd_create");
	for (int i = 0; i < NTHREADS; i++)
		check(thrd_join(tid[i], NULL), "thrd_join");	/* NULL: ignore result */

	printf("  counter = %ld (expected %d)\n", c.value, NTHREADS * INCREMENTS);
	mtx_destroy(&c.lock);	/* returns void */
}

/* ------------------------------------------------------------------ */
/* Part 3: condition variables, a producer/consumer queue             */
/* ------------------------------------------------------------------ */

#define QSIZE      4	/* small on purpose, so the producer must wait */
#define NITEMS     20
#define NCONSUMERS 2
#define STOP       (-1)	/* "no more items" marker, one per consumer */

/*
 * A bounded queue (ring buffer) shared by one producer and several
 * consumers. A condition variable lets a thread SLEEP until something
 * changes, instead of spinning in a loop checking a flag.
 *
 *   not_full   the producer waits here while the queue is full
 *   not_empty  consumers wait here while the queue is empty
 *
 * Both are always used together with the mutex that protects the queue.
 */
struct queue {
	mtx_t lock;
	cnd_t not_full, not_empty;
	int items[QSIZE];
	int head, count;
};

static void queue_push(struct queue *q, int v)
{
	check(mtx_lock(&q->lock), "mtx_lock");
	/*
	 * ALWAYS wait in a while loop, never an if: cnd_wait can wake up
	 * without anyone signalling (a "spurious wakeup"), and another
	 * thread may have refilled the queue before we got the lock back.
	 *
	 * cnd_wait atomically unlocks the mutex and sleeps; when it wakes
	 * it locks the mutex again before returning.
	 */
	while (q->count == QSIZE)
		check(cnd_wait(&q->not_full, &q->lock), "cnd_wait");

	q->items[(q->head + q->count) % QSIZE] = v;
	q->count++;

	check(cnd_signal(&q->not_empty), "cnd_signal");	/* wake one consumer */
	check(mtx_unlock(&q->lock), "mtx_unlock");
}

static int queue_pop(struct queue *q)
{
	int v;

	check(mtx_lock(&q->lock), "mtx_lock");
	while (q->count == 0)
		check(cnd_wait(&q->not_empty, &q->lock), "cnd_wait");

	v = q->items[q->head];
	q->head = (q->head + 1) % QSIZE;
	q->count--;

	check(cnd_signal(&q->not_full), "cnd_signal");	/* wake the producer */
	check(mtx_unlock(&q->lock), "mtx_unlock");
	return v;
}

static int producer(void *p)
{
	struct queue *q = p;

	for (int i = 1; i <= NITEMS; i++)
		queue_push(q, i);
	for (int i = 0; i < NCONSUMERS; i++)
		queue_push(q, STOP);	/* tell every consumer to finish */
	return 0;
}

/* Each consumer gets the shared queue plus its own result slot. */
struct consumer_arg {
	struct queue *q;
	int id;
	long sum;
	int taken;
};

static int consumer(void *p)
{
	struct consumer_arg *a = p;
	int v;

	while ((v = queue_pop(a->q)) != STOP) {
		a->sum += v;
		a->taken++;
		msleep(1);	/* pretend each item takes some work */
	}
	printf("  consumer %d: took %d items, sum %ld\n", a->id, a->taken, a->sum);
	return 0;
}

static void part3_condvar(void)
{
	struct queue q = { .head = 0, .count = 0 };
	struct consumer_arg args[NCONSUMERS];
	thrd_t prod, cons[NCONSUMERS];
	long total = 0;

	printf("3. producer/consumer: %d items, queue of %d, %d consumers\n",
	       NITEMS, QSIZE, NCONSUMERS);

	check(mtx_init(&q.lock, mtx_plain), "mtx_init");
	check(cnd_init(&q.not_full), "cnd_init");
	check(cnd_init(&q.not_empty), "cnd_init");

	for (int i = 0; i < NCONSUMERS; i++) {
		args[i] = (struct consumer_arg){ .q = &q, .id = i };
		check(thrd_create(&cons[i], consumer, &args[i]), "thrd_create");
	}
	check(thrd_create(&prod, producer, &q), "thrd_create");

	check(thrd_join(prod, NULL), "thrd_join");
	for (int i = 0; i < NCONSUMERS; i++) {
		check(thrd_join(cons[i], NULL), "thrd_join");
		total += args[i].sum;
	}
	printf("  total = %ld (expected %d)\n", total, NITEMS * (NITEMS + 1) / 2);

	cnd_destroy(&q.not_empty);
	cnd_destroy(&q.not_full);
	mtx_destroy(&q.lock);
}

/* ------------------------------------------------------------------ */
/* Part 4: thread_local, tss_t and call_once                          */
/* ------------------------------------------------------------------ */

/*
 * thread_local (a macro for _Thread_local) gives EVERY thread its own
 * copy of a global or static variable. Simple and fast, but nothing is
 * run when the thread exits, so it suits plain values, not pointers to
 * memory that needs freeing.
 */
static thread_local int calls_in_this_thread;

/*
 * tss_t ("thread-specific storage") is the heavier version: each thread
 * stores its own void * under one key, AND you give a destructor that
 * runs automatically on that pointer when the thread exits. Use it for
 * per-thread buffers that must be freed. (pthreads: pthread_key_t.)
 */
static tss_t buf_key;

/*
 * call_once runs a function exactly once, however many threads call it
 * at the same time; the others wait until it has finished. Perfect for
 * lazy one-time setup, such as creating buf_key here.
 */
static once_flag key_once = ONCE_FLAG_INIT;

static void free_buf(void *p)	/* the tss destructor */
{
	printf("  destructor: freeing \"%s\"\n", (char *)p);
	free(p);
}

static void make_key(void)	/* runs once, in whichever thread is first */
{
	check(tss_create(&buf_key, free_buf), "tss_create");
	puts("  call_once: key created (printed once)");
}

/* Returns this thread's private buffer, creating it on first use. */
static char *thread_buf(int id)
{
	char *b;

	call_once(&key_once, make_key);
	b = tss_get(buf_key);	/* NULL the first time in each thread */
	if (!b) {
		b = malloc(32);
		if (!b) {
			fputs("malloc failed\n", stderr);
			exit(EXIT_FAILURE);
		}
		snprintf(b, 32, "buffer of thread %d", id);
		check(tss_set(buf_key, b), "tss_set");
	}
	return b;
}

static int tls_worker(void *p)
{
	int id = *(const int *)p;

	for (int i = 0; i <= id; i++) {	/* thread N calls N+1 times */
		calls_in_this_thread++;		/* no lock: it's private */
		(void)thread_buf(id);		/* same buffer every time */
	}
	printf("  thread %d: %d call(s), uses \"%s\"\n",
	       id, calls_in_this_thread, thread_buf(id));
	/* no free() here: free_buf runs for us when the thread returns */
	return 0;
}

static void part4_thread_local(void)
{
	thrd_t tid[3];
	int ids[3] = { 0, 1, 2 };	/* stable addresses, one per thread */

	puts("4. thread_local, tss_t with destructor, call_once");
	for (int i = 0; i < 3; i++)
		check(thrd_create(&tid[i], tls_worker, &ids[i]), "thrd_create");
	for (int i = 0; i < 3; i++)
		check(thrd_join(tid[i], NULL), "thrd_join");

	/* main never used thread_buf, so it never got a buffer of its own */
	printf("  main: calls_in_this_thread = %d (its own copy)\n",
	       calls_in_this_thread);
	tss_delete(buf_key);
}

/* ------------------------------------------------------------------ */
/* Part 5: timed lock, and a detached thread we wait for properly     */
/* ------------------------------------------------------------------ */

static mtx_t slow_lock;	/* created with mtx_timed */

static int impatient(void *p)
{
	struct timespec dl = deadline_in(100);	/* give up after 100 ms */
	int rc;

	(void)p;
	rc = mtx_timedlock(&slow_lock, &dl);
	if (rc == thrd_timedout) {
		puts("  impatient thread: gave up after 100 ms (thrd_timedout)");
		return 1;
	}
	check(rc, "mtx_timedlock");
	puts("  impatient thread: got the lock");
	check(mtx_unlock(&slow_lock), "mtx_unlock");
	return 0;
}

/*
 * A detached thread can't be joined, so main needs another way to know
 * it has finished: here a "done" flag protected by a mutex and a
 * condition variable. (Returning from main kills every thread that is
 * still running, mid-work.)
 */
struct done_signal {
	mtx_t lock;
	cnd_t cv;
	int done;
};

static int background(void *p)
{
	struct done_signal *d = p;

	msleep(50);	/* pretend to work */
	puts("  detached thread: finished, telling main");

	check(mtx_lock(&d->lock), "mtx_lock");
	d->done = 1;
	check(cnd_signal(&d->cv), "cnd_signal");
	check(mtx_unlock(&d->lock), "mtx_unlock");
	return 0;	/* a detached thread frees its own resources here */
}

static void part5_timed_detach(void)
{
	struct done_signal d = { .done = 0 };
	thrd_t t;
	int status;

	puts("5. mtx_timedlock with a deadline, and a detached thread");

	/* a) main holds the lock too long, so the other thread times out */
	check(mtx_init(&slow_lock, mtx_timed), "mtx_init");
	check(mtx_lock(&slow_lock), "mtx_lock");
	check(thrd_create(&t, impatient, NULL), "thrd_create");
	msleep(300);	/* hold the lock for 300 ms */
	check(mtx_unlock(&slow_lock), "mtx_unlock");
	check(thrd_join(t, &status), "thrd_join");
	printf("  impatient thread returned %d\n", status);
	mtx_destroy(&slow_lock);

	/* b) fire-and-forget thread, but we still wait until it's done */
	check(mtx_init(&d.lock, mtx_plain), "mtx_init");
	check(cnd_init(&d.cv), "cnd_init");
	check(thrd_create(&t, background, &d), "thrd_create");
	check(thrd_detach(t), "thrd_detach");	/* thrd_join(t) is now invalid */

	check(mtx_lock(&d.lock), "mtx_lock");
	while (!d.done) {
		/* cnd_timedwait: like cnd_wait, but with a safety deadline */
		struct timespec dl = deadline_in(2000);

		if (cnd_timedwait(&d.cv, &d.lock, &dl) == thrd_timedout) {
			fputs("background thread never finished\n", stderr);
			exit(EXIT_FAILURE);
		}
	}
	check(mtx_unlock(&d.lock), "mtx_unlock");
	puts("  main: detached thread is done");

	/*
	 * Because d lives on main's stack, destroying it is only safe once
	 * the thread no longer touches it. background() unlocks d.lock as
	 * its last use, and we locked it after that, so we're fine.
	 */
	cnd_destroy(&d.cv);
	mtx_destroy(&d.lock);
}

int main(void)
{
	part1_create_join();
	putchar('\n');
	part2_mutex();
	putchar('\n');
	part3_condvar();
	putchar('\n');
	part4_thread_local();
	putchar('\n');
	part5_timed_detach();
	return EXIT_SUCCESS;
}
