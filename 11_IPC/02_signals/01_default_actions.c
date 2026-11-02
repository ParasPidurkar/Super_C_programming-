// SPDX-License-Identifier: GPL-2.0
/*
 * 01_default_actions - what a signal does when nobody handles it.
 *
 * Every signal has a default action, one of:
 *
 *   terminate   end the process            (SIGTERM, SIGINT, SIGKILL, ...)
 *   core        end it and write a core    (SIGQUIT, SIGSEGV, SIGABRT, ...)
 *   ignore      nothing happens            (SIGCHLD, SIGWINCH, SIGURG)
 *   stop        pause the process          (SIGSTOP, SIGTSTP)
 *   continue    resume a stopped process   (SIGCONT)
 *
 * For each signal we fork a child that just waits, send it the signal,
 * and decode what waitpid() reports. That is exactly how a shell knows
 * to print "Killed" or "Segmentation fault (core dumped)".
 */
#define _XOPEN_SOURCE 700

#include <sys/resource.h>
#include <sys/wait.h>

#include "common.h"

static pid_t spawn(void)
{
	pid_t pid = fork();

	if (pid < 0)
		die("fork");
	if (pid == 0) {
		/* Don't litter the folder with core files from SIGQUIT. */
		struct rlimit none = { 0, 0 };
		int sig;

		setrlimit(RLIMIT_CORE, &none);
		/*
		 * "Ignore" is inherited across fork() and exec(). Scripts and
		 * other non-interactive shells start background jobs with
		 * SIGINT and SIGQUIT ignored, so reset everything to the
		 * default to see the real defaults.
		 */
		for (sig = 1; sig <= SIGRTMAX; sig++)
			if (sig != SIGKILL && sig != SIGSTOP)
				signal(sig, SIG_DFL);
		for (;;)
			pause();	/* sleep until a signal arrives */
	}
	msleep(20);	/* let the child reach pause() */
	return pid;
}

/* Decode a waitpid() status the way a shell would. */
static void describe(int status)
{
	if (WIFEXITED(status))
		printf("exited with code %d\n", WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		printf("killed by signal %d (%s)%s\n", WTERMSIG(status),
		       strsignal(WTERMSIG(status)),
#ifdef WCOREDUMP
		       WCOREDUMP(status) ? ", core dumped" : "");
#else
		       "");
#endif
	else if (WIFSTOPPED(status))
		printf("stopped by signal %d (%s)\n", WSTOPSIG(status),
		       strsignal(WSTOPSIG(status)));
	else if (WIFCONTINUED(status))
		puts("continued");
}

static void send_and_report(int sig, const char *name, int ignored)
{
	pid_t pid = spawn();
	int status;

	printf("  %-8s -> ", name);
	if (kill(pid, sig))
		die("kill");

	if (ignored) {
		/* Give it time to die; if it is still alive, it ignored us. */
		msleep(50);
		if (waitpid(pid, &status, WNOHANG) == 0) {
			puts("ignored, child still running");
			kill(pid, SIGKILL);
			waitpid(pid, &status, 0);
			return;
		}
	} else {
		/*
		 * Wait as long as it takes: writing a core dump (or handing it
		 * to a crash reporter like Ubuntu's apport) can take a while.
		 */
		waitpid(pid, &status, 0);
	}
	describe(status);
}

int main(void)
{
	pid_t pid;
	int status;

	setvbuf(stdout, NULL, _IOLBF, 0);

	puts("1. signals whose default action ends the process");
	send_and_report(SIGTERM, "SIGTERM", 0);	/* plain `kill <pid>` */
	send_and_report(SIGINT, "SIGINT", 0);	/* Ctrl-C */
	send_and_report(SIGKILL, "SIGKILL", 0);	/* `kill -9`, can't be caught */
	send_and_report(SIGUSR1, "SIGUSR1", 0);	/* even the "user" signals */
	send_and_report(SIGQUIT, "SIGQUIT", 0);	/* Ctrl-\, default = core */

	puts("2. signals ignored by default");
	send_and_report(SIGCHLD, "SIGCHLD", 1);
	send_and_report(SIGWINCH, "SIGWINCH", 1);

	puts("3. stop and continue (what Ctrl-Z and `fg` do)");
	pid = spawn();
	kill(pid, SIGSTOP);
	/* WUNTRACED: also report children that stopped, not only died. */
	waitpid(pid, &status, WUNTRACED);
	printf("  %-8s -> ", "SIGSTOP");
	describe(status);

	kill(pid, SIGCONT);
	/* WCONTINUED: also report children that were resumed. */
	waitpid(pid, &status, WCONTINUED);
	printf("  %-8s -> ", "SIGCONT");
	describe(status);

	kill(pid, SIGTERM);
	waitpid(pid, &status, 0);
	printf("  %-8s -> ", "SIGTERM");
	describe(status);

	puts("4. the exit status a shell shows: 128 + signal number");
	printf("  SIGINT -> $? = %d, SIGKILL -> $? = %d\n", 128 + SIGINT,
	       128 + SIGKILL);
	return EXIT_SUCCESS;
}
