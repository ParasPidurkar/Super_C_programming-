// SPDX-License-Identifier: GPL-2.0
/*
 * 04_sigchld - zombies, SIGCHLD and reaping children correctly.
 *
 *   1. A child that has exited but not been waited for is a zombie:
 *      it holds a PID and an exit status until the parent collects it.
 *   2. The kernel sends SIGCHLD to the parent when a child exits. Since
 *      standard signals merge, one SIGCHLD can stand for several dead
 *      children, so the handler must loop on waitpid(WNOHANG).
 *   3. To wait for the handler without a race, block the signal and use
 *      sigsuspend(), never "check flag, then pause()".
 */
#define _XOPEN_SOURCE 700

#include <sys/wait.h>

#include "common.h"

#define NCHILD 5

static volatile sig_atomic_t reaped, handler_runs;
static volatile sig_atomic_t dead_pid[NCHILD], dead_code[NCHILD];

static void on_chld(int sig)
{
	int saved_errno = errno;	/* waitpid() may change errno */
	int status;
	pid_t pid;

	(void)sig;
	handler_runs++;
	/* Reap every child that is done, not just one. */
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		if (reaped < NCHILD) {
			dead_pid[reaped] = pid;
			dead_code[reaped] = WIFEXITED(status) ?
					    WEXITSTATUS(status) : -1;
		}
		reaped++;
	}
	errno = saved_errno;
}

/* Linux-specific: the state letter in /proc/<pid>/stat (R, S, Z, ...). */
static char proc_state(pid_t pid)
{
	char path[64], buf[256], *p;
	FILE *f;

	snprintf(path, sizeof(path), "/proc/%d/stat", (int)pid);
	f = fopen(path, "r");
	if (!f)
		return '-';	/* no such process any more */
	p = fgets(buf, sizeof(buf), f);
	fclose(f);
	if (!p || !(p = strrchr(buf, ')')))
		return '?';
	return p[2];	/* "pid (comm) S ..." */
}

static void part1_zombie(void)
{
	pid_t pid;
	int status;

	puts("1. a zombie: the child exited, nobody waited for it");
	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		_exit(7);

	msleep(100);
	printf("  child %d state: %c   (Z = zombie, shown as <defunct> by ps)\n",
	       (int)pid, proc_state(pid));
	waitpid(pid, &status, 0);
	printf("  after waitpid: state %c, exit code %d\n", proc_state(pid),
	       WEXITSTATUS(status));
}

static void part2_reap(void)
{
	sigset_t block, old;
	pid_t pid;
	int i;

	printf("2. %d children exit at almost the same moment\n", NCHILD);

	/*
	 * Block SIGCHLD BEFORE forking. Otherwise a child could exit and
	 * the handler could run before we start waiting, and we would
	 * sleep forever waiting for a signal that already came.
	 */
	sigemptyset(&block);
	sigaddset(&block, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &block, &old))
		die("sigprocmask");
	/* SA_NOCLDSTOP: don't tell us when children merely stop. */
	install(SIGCHLD, on_chld, SA_RESTART | SA_NOCLDSTOP);

	for (i = 0; i < NCHILD; i++) {
		pid = fork();
		if (pid < 0)
			die("fork");
		if (pid == 0) {
			msleep(100);
			_exit(10 + i);
		}
	}

	/*
	 * All children are dead by now, but SIGCHLD is still blocked, so the
	 * kernel holds it as pending. Five exits, ONE pending SIGCHLD.
	 */
	msleep(300);

	/*
	 * sigsuspend() atomically swaps in the old mask (SIGCHLD unblocked)
	 * and sleeps. The handler runs, sigsuspend() returns, and SIGCHLD
	 * is blocked again while we check the counter.
	 */
	while (reaped < NCHILD)
		sigsuspend(&old);

	if (sigprocmask(SIG_SETMASK, &old, NULL))
		die("sigprocmask");

	printf("  handler ran %d time(s) but reaped all %d children:\n",
	       (int)handler_runs, (int)reaped);
	for (i = 0; i < NCHILD; i++)
		printf("    pid %d exited with code %d\n", (int)dead_pid[i],
		       (int)dead_code[i]);
}

int main(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	part1_zombie();
	part2_reap();
	return EXIT_SUCCESS;
}
