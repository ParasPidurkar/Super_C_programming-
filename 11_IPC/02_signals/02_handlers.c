// SPDX-License-Identifier: GPL-2.0
/*
 * 02_handlers - catching signals with sigaction().
 *
 *   1. A handler with SA_SIGINFO learns who sent the signal and how.
 *   2. SIGKILL and SIGSTOP can never be caught, blocked or ignored.
 *   3. A handler interrupts a blocking call: without SA_RESTART the call
 *      fails with EINTR, with SA_RESTART the kernel restarts it.
 *   4. SIG_IGN makes a signal do nothing at all.
 *
 * The golden rule: a handler only records what happened in a
 * volatile sig_atomic_t, and the main program acts on it.
 */
#define _XOPEN_SOURCE 700

#include <sys/time.h>
#include <sys/wait.h>

#include "common.h"

/* Written by the handler, read by main(). */
static volatile sig_atomic_t got_sig, sender_pid, sender_code, sender_value;

static void on_usr1(int sig, siginfo_t *si, void *ctx)
{
	(void)ctx;
	sender_pid = si->si_pid;
	sender_code = si->si_code;
	sender_value = si->si_code == SI_QUEUE ? si->si_value.sival_int : 0;
	got_sig = sig;
}

static void on_alarm(int sig)
{
	(void)sig;
	say("    [handler] SIGALRM ran\n");
}

static const char *code_name(int code)
{
	switch (code) {
	case SI_USER:	return "SI_USER (kill)";
	case SI_QUEUE:	return "SI_QUEUE (sigqueue)";
	default:	return "other";
	}
}

static void wait_for_usr1(void)
{
	while (!got_sig)
		msleep(10);	/* the handler sets got_sig */
	printf("  caught %s from pid %d (%s), si_code %s, value %d\n",
	       strsignal(got_sig), (int)sender_pid,
	       sender_pid == getpid() ? "me" : "child",
	       code_name(sender_code), (int)sender_value);
	got_sig = 0;
}

static void part1_siginfo(void)
{
	struct sigaction sa;
	union sigval val = { .sival_int = 42 };
	pid_t pid;

	puts("1. a handler that knows who sent the signal");
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = on_usr1;	/* 3-argument handler ... */
	sa.sa_flags = SA_SIGINFO;	/* ... needs this flag */
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGUSR1, &sa, NULL))
		die("sigaction");

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0) {
		kill(getppid(), SIGUSR1);
		_exit(EXIT_SUCCESS);
	}
	wait_for_usr1();
	waitpid(pid, NULL, 0);

	/* sigqueue() is kill() plus one int (or pointer) of data. */
	if (sigqueue(getpid(), SIGUSR1, val))
		die("sigqueue");
	wait_for_usr1();
}

static void part2_uncatchable(void)
{
	struct sigaction sa;

	puts("2. SIGKILL and SIGSTOP cannot be caught");
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGKILL, &sa, NULL))
		printf("  sigaction(SIGKILL) -> %s\n", strerror(errno));
	if (sigaction(SIGSTOP, &sa, NULL))
		printf("  sigaction(SIGSTOP) -> %s\n", strerror(errno));
}

/*
 * Block in read() on a pipe. A timer fires SIGALRM after 200 ms; the
 * child writes data after 500 ms. What read() does at 200 ms depends on
 * SA_RESTART.
 */
static void blocking_read(int flags, const char *label)
{
	struct itimerval t = { .it_value = { 0, 200000 } };
	char buf[16];
	int fds[2];
	ssize_t n;
	pid_t pid;

	printf("  %s:\n", label);
	install(SIGALRM, on_alarm, flags);
	if (pipe(fds))
		die("pipe");

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0) {
		close(fds[0]);
		msleep(500);
		if (write(fds[1], "data", 5) < 0)
			_exit(EXIT_FAILURE);
		_exit(EXIT_SUCCESS);
	}
	close(fds[1]);

	if (setitimer(ITIMER_REAL, &t, NULL))
		die("setitimer");
	n = read(fds[0], buf, sizeof(buf));
	if (n < 0)
		printf("    read() -> -1, errno %s\n",
		       errno == EINTR ? "EINTR" : strerror(errno));
	else
		printf("    read() -> %zd bytes \"%s\"\n", n, buf);

	close(fds[0]);
	waitpid(pid, NULL, 0);
}

static void part3_restart(void)
{
	puts("3. a signal interrupts a blocking read()");
	blocking_read(0, "without SA_RESTART");
	blocking_read(SA_RESTART, "with SA_RESTART");
}

static void part4_ignore(void)
{
	puts("4. SIG_IGN: the signal is thrown away");
	install(SIGUSR2, SIG_IGN, 0);
	raise(SIGUSR2);		/* default action would kill us */
	puts("  raise(SIGUSR2) -> still alive");
	install(SIGUSR2, SIG_DFL, 0);	/* back to the default */
}

int main(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	part1_siginfo();
	part2_uncatchable();
	part3_restart();
	part4_ignore();
	return EXIT_SUCCESS;
}
