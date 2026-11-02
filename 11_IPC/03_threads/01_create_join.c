// SPDX-License-Identifier: GPL-2.0
/*
 * 01_create_join - start threads, pass them arguments, get results back.
 *
 * WHAT IS A THREAD?
 * A thread is a second flow of execution inside the SAME process.
 *
 *   Shared by all threads:     code, global/static variables, the heap
 *                              (malloc), open files, signal handlers
 *   Private to each thread:    its own stack (local variables), CPU
 *                              registers, errno, signal mask
 *
 * Because memory is shared, threads can exchange data through plain
 * variables. No pipes or message queues are needed, unlike processes.
 *
 * LIFECYCLE
 *   pthread_create()  starts a new thread running a function you choose
 *   ...both threads now run at the same time (truly in parallel on a
 *      multi-core CPU), and their order is NOT predictable...
 *   pthread_join()    waits until that thread finishes and collects
 *                     its return value; like waitpid() for a child
 *
 * Every thread must end up either JOINED or DETACHED (part 3), or its
 * resources leak, much like a zombie process.
 */
#define _POSIX_C_SOURCE 200809L

#include "common.h"

#define NTHREADS 4

/*
 * A thread function takes exactly ONE argument, a void *. To pass more
 * than one value, put them all in a struct and pass its address.
 * The same struct is a handy place for the thread to write its result.
 */
struct job {
	int id;
	long from, to;	/* input: sum the range [from, to] */
	long result;	/* output: written by the thread, read after join */
};

/*
 * Thread entry point. The signature is fixed by pthreads:
 *     void *function(void *arg)
 * Whatever was passed as the last argument of pthread_create() arrives
 * here as @p; we convert it back to the real type.
 */
static void *sum_range(void *p)
{
	struct job *j = p;	/* void * converts to any pointer in C */
	long i, s = 0;		/* locals live on THIS thread's own stack */

	for (i = j->from; i <= j->to; i++)
		s += i;

	/*
	 * Writing to j->result is safe without a lock: each thread got a
	 * DIFFERENT struct, and main only reads it after pthread_join(),
	 * which guarantees the thread's writes are visible.
	 */
	j->result = s;
	printf("  thread %d: sum(%ld..%ld) = %ld\n", j->id, j->from, j->to, s);
	return NULL;	/* returning from the function ends the thread */
}

/*
 * Returning a value. The thread's return value is a void *, which
 * pthread_join() hands back to the caller.
 *
 * The pointer must still be valid AFTER the thread is gone, so it may
 * NOT point at a local variable (that stack is freed when the thread
 * exits). We malloc() it; whoever joins the thread must free() it.
 */
static void *square(void *p)
{
	long *out = malloc(sizeof(*out));
	long n = (long)(*(int *)p);	/* read the int that main passed in */

	if (!out)
		die("malloc");
	*out = n * n;
	return out;	/* same as calling pthread_exit(out) */
}

/* Used in part 3: a thread nobody waits for. */
static void *background(void *p)
{
	(void)p;	/* unused argument; the cast silences the warning */
	msleep(50);	/* pretend to work for a while */
	puts("  detached thread: finished, nobody will join me");
	return NULL;	/* a detached thread frees its own resources here */
}

int main(void)
{
	pthread_t tid[NTHREADS], t;	/* thread IDs, filled in by create */
	pthread_attr_t attr;		/* optional settings for a new thread */
	struct job jobs[NTHREADS];	/* one argument struct PER thread */
	long total = 0, per = 1000000 / NTHREADS;
	void *ret;
	int i, seven = 7;

	/* ---- Part 1: split one big job across several threads ---- */
	printf("1. split sum(1..1000000) across %d threads\n", NTHREADS);
	for (i = 0; i < NTHREADS; i++) {
		/*
		 * Each thread gets its OWN struct. A classic bug is to pass &i:
		 * the loop changes i before the new thread gets around to
		 * reading it, so several threads would see the same number.
		 */
		jobs[i].id = i;
		jobs[i].from = i * per + 1;
		jobs[i].to = (i + 1) * per;

		/*
		 * pthread_create(&id, attributes, function, argument)
		 *   &tid[i]    receives the new thread's ID
		 *   NULL       default attributes (joinable, default stack size)
		 *   sum_range  the function the new thread starts in
		 *   &jobs[i]   the void * argument given to that function
		 * The call returns right away; the new thread is already
		 * running (or about to) alongside main.
		 */
		check(pthread_create(&tid[i], NULL, sum_range, &jobs[i]),
		      "pthread_create");
	}

	/*
	 * Wait for every thread. The order of the "thread N:" lines above
	 * can change from run to run; joining in order 0,1,2,3 just means
	 * main WAITS in that order, not that the threads finish in it.
	 */
	for (i = 0; i < NTHREADS; i++) {
		check(pthread_join(tid[i], NULL), "pthread_join");
		total += jobs[i].result;	/* safe: thread i has finished */
	}
	printf("  total = %ld (expected %ld)\n", total, 1000000L * 1000001L / 2);

	/* ---- Part 2: get a value back through pthread_join() ---- */
	puts("2. get a heap-allocated return value through pthread_join()");
	/*
	 * Passing &seven is fine here, unlike &i in the loop above: seven
	 * never changes, and it lives until main returns.
	 */
	check(pthread_create(&t, NULL, square, &seven), "pthread_create");
	/* pthread_join stores the thread's return value into ret. */
	check(pthread_join(t, &ret), "pthread_join");
	printf("  square(%d) = %ld\n", seven, *(long *)ret);
	free(ret);	/* the thread malloc'd it; we own it now */

	/* ---- Part 3: detached threads ---- */
	puts("3. a detached thread cleans itself up; it can't be joined");
	/*
	 * Thread attributes let you change how a thread is created:
	 * detached or joinable, stack size, scheduling policy...
	 * Init the attribute object, set what you need, use it, destroy it.
	 */
	check(pthread_attr_init(&attr), "pthread_attr_init");
	check(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED),
	      "pthread_attr_setdetachstate");
	check(pthread_create(&t, &attr, background, NULL), "pthread_create");
	pthread_attr_destroy(&attr);	/* the thread keeps its own copy */
	/*
	 * Alternatively: create it normally and call pthread_detach(t).
	 *
	 * Joining a detached thread is undefined behaviour: it may already
	 * have exited, and its ID may have been reused. Use detached
	 * threads for "fire and forget" work whose result you don't need.
	 */

	/*
	 * Returning from main() calls exit(), which ends the WHOLE process,
	 * killing every thread still running, mid-work. So give the
	 * detached thread time to finish. (A sleep is fine in a demo; real
	 * code would wait on a semaphore, as example 04 does. Calling
	 * pthread_exit(NULL) instead of return would also work: it ends
	 * only main's thread, and the process lives on until the last
	 * thread ends.)
	 */
	msleep(100);
	return EXIT_SUCCESS;
}
