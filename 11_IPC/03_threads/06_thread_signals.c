// SPDX-License-Identifier: GPL-2.0
/*
 * 06_thread_signals - handling signals in a multithreaded program.
 *
 * WHAT CHANGES WITH THREADS
 *   - Signal HANDLERS (sigaction) are per-PROCESS: shared by all threads.
 *   - The signal MASK (blocked signals) is per-THREAD.
 *   - A signal sent to the process (kill, Ctrl-C) is delivered to ANY
 *     ONE thread that has not blocked it, chosen by the kernel.
 *
 * So with a normal handler you can't know which thread gets interrupted,
 * maybe one holding a mutex the handler then needs, which deadlocks.
 * And handlers may only call async-signal-safe functions (no printf,
 * malloc or pthread_mutex_lock).
 *
 * THE CLEAN PATTERN: a dedicated signal thread
 *   1. In main, BEFORE creating any thread, block the signals with
 *      pthread_sigmask(). New threads inherit the creator's mask, so
 *      every thread now has them blocked: nobody gets interrupted.
 *   2. Blocked signals stay "pending". One thread picks them up by
 *      calling sigwait(), which sleeps until one of them is pending,
 *      removes it, and returns its number.
 *   3. That thread is ordinary code, not a handler, so it may lock
 *      mutexes, printf, and tell other threads what to do.
 */
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdatomic.h>

#include "common.h"

#define WORKERS 2

/*
 * Flags the signal thread sets and the workers read. atomic_* makes
 * those reads and writes safe between threads (see example 02).
 */
static atomic_bool stop;
static atomic_int reloads;

/* The ONLY thread that deals with signals. */
static void *signal_thread(void *p)
{
	sigset_t *set = p;	/* the signals to wait for, from main */
	int sig;

	for (;;) {
		/*
		 * Sleep until one of the signals in *set is pending, then
		 * take it (it's consumed, so no handler runs) and store its
		 * number in sig. Requires the signals to be BLOCKED;
		 * otherwise they'd be delivered the normal way instead.
		 */
		check(sigwait(set, &sig), "sigwait");

		if (sig == SIGHUP) {
			/*
			 * Unix convention: SIGHUP = "reload your configuration".
			 * Real code would re-read a config file here; since this
			 * is a normal thread, it may lock mutexes to do that.
			 */
			atomic_fetch_add(&reloads, 1);
			puts("  signal thread: SIGHUP -> reload config");
		} else {
			/*
			 * SIGTERM / SIGINT = "please shut down". We don't exit()
			 * here; we ask the workers to stop, so they can finish
			 * cleanly (close files, free memory...).
			 */
			printf("  signal thread: %s -> asking workers to stop\n",
			       sig == SIGTERM ? "SIGTERM" : "SIGINT");
			atomic_store(&stop, 1);
			return NULL;
		}
	}
}

/*
 * A worker never sees a signal: they're blocked in its mask. It just
 * checks the stop flag between units of work.
 */
static void *worker(void *p)
{
	int id = *(int *)p, ticks = 0;

	while (!atomic_load(&stop)) {
		msleep(20);	/* one unit of "work" */
		ticks++;
	}
	printf("  worker %d: stopped cleanly after %d ticks\n", id, ticks);
	return NULL;
}

int main(void)
{
	pthread_t sig_tid, tid[WORKERS];
	int ids[WORKERS], i;
	sigset_t set;

	/* Build the set of signals we want to handle ourselves. */
	sigemptyset(&set);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);

	/*
	 * STEP 1: block them in main's thread FIRST. Every thread created
	 * after this line inherits the blocked mask.
	 *
	 * Use pthread_sigmask, not sigprocmask: sigprocmask's behaviour in
	 * a multithreaded process is unspecified by POSIX.
	 */
	check(pthread_sigmask(SIG_BLOCK, &set, NULL), "pthread_sigmask");

	/*
	 * STEP 2: start the signal thread. &set stays valid because main
	 * outlives every thread (it joins them all below).
	 */
	check(pthread_create(&sig_tid, NULL, signal_thread, &set),
	      "pthread_create");
	for (i = 0; i < WORKERS; i++) {
		ids[i] = i;
		check(pthread_create(&tid[i], NULL, worker, &ids[i]),
		      "pthread_create");
	}

	/*
	 * Simulate the outside world. kill(getpid(), ...) sends to the
	 * whole PROCESS, exactly like "kill -HUP <pid>" from a shell would.
	 * Every thread has these blocked, so the signal stays pending until
	 * the signal thread's sigwait() takes it.
	 *
	 * (pthread_kill(tid, sig) would instead target one specific thread.)
	 */
	puts("main: kill(getpid(), SIGHUP) twice, then SIGTERM");
	msleep(50);
	kill(getpid(), SIGHUP);
	msleep(50);
	kill(getpid(), SIGHUP);
	msleep(50);
	kill(getpid(), SIGTERM);

	/* STEP 3: orderly shutdown. Wait for everyone to finish. */
	check(pthread_join(sig_tid, NULL), "pthread_join");
	for (i = 0; i < WORKERS; i++)
		check(pthread_join(tid[i], NULL), "pthread_join");
	printf("main: all threads joined, %d reload(s) handled\n",
	       atomic_load(&reloads));
	return EXIT_SUCCESS;
}
