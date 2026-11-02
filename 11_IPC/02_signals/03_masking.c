// SPDX-License-Identifier: GPL-2.0
/*
 * 03_masking - blocking signals, pending signals, and why standard
 * signals get lost while real-time signals don't.
 *
 * A blocked signal is not thrown away: it becomes "pending" and is
 * delivered the moment it is unblocked. But the kernel keeps only ONE
 * pending bit per standard signal, so five SIGUSR1s sent while blocked
 * arrive as one. Real-time signals (SIGRTMIN..SIGRTMAX) are queued: all
 * five arrive, in order, each with its own data.
 */
#define _XOPEN_SOURCE 700

#include "common.h"

#define N 5

static volatile sig_atomic_t usr1_count, rt_count;
static volatile sig_atomic_t rt_values[N];

static void on_usr1(int sig)
{
	(void)sig;
	usr1_count++;
}

static void on_rt(int sig, siginfo_t *si, void *ctx)
{
	(void)sig;
	(void)ctx;
	if (rt_count < N)
		rt_values[rt_count] = si->si_value.sival_int;
	rt_count++;
}

static void show_pending(void)
{
	sigset_t pending;

	if (sigpending(&pending))
		die("sigpending");
	printf("  pending: SIGUSR1=%s SIGRTMIN=%s\n",
	       sigismember(&pending, SIGUSR1) ? "yes" : "no",
	       sigismember(&pending, SIGRTMIN) ? "yes" : "no");
}

int main(void)
{
	struct sigaction sa;
	union sigval val;
	sigset_t block;
	int i;

	install(SIGUSR1, on_usr1, 0);

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = on_rt;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL))
		die("sigaction");

	printf("  real-time signals on this system: SIGRTMIN=%d .. SIGRTMAX=%d\n",
	       SIGRTMIN, SIGRTMAX);

	puts("1. block SIGUSR1 and SIGRTMIN");
	sigemptyset(&block);
	sigaddset(&block, SIGUSR1);
	sigaddset(&block, SIGRTMIN);
	if (sigprocmask(SIG_BLOCK, &block, NULL))
		die("sigprocmask");
	show_pending();

	printf("2. send each of them %d times while blocked\n", N);
	for (i = 1; i <= N; i++) {
		kill(getpid(), SIGUSR1);
		val.sival_int = i * 10;
		if (sigqueue(getpid(), SIGRTMIN, val))
			die("sigqueue");
	}
	printf("  handlers run so far: SIGUSR1=%d SIGRTMIN=%d\n",
	       (int)usr1_count, (int)rt_count);
	show_pending();

	puts("3. unblock: everything pending is delivered right away");
	if (sigprocmask(SIG_UNBLOCK, &block, NULL))
		die("sigprocmask");
	printf("  SIGUSR1  handler ran %d time(s) for %d sent (merged)\n",
	       (int)usr1_count, N);
	printf("  SIGRTMIN handler ran %d time(s) for %d sent, values:",
	       (int)rt_count, N);
	for (i = 0; i < N && i < rt_count; i++)
		printf(" %d", (int)rt_values[i]);
	putchar('\n');
	show_pending();
	return EXIT_SUCCESS;
}
