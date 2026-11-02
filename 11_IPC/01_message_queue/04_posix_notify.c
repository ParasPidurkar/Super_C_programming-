// SPDX-License-Identifier: GPL-2.0
/*
 * 04_posix_notify - get told when a message arrives instead of blocking.
 *
 * mq_notify() asks the kernel to signal us when the queue goes from
 * empty to non-empty. The rules that trip people up:
 *
 *   - It is one-shot. After it fires, you must register again.
 *   - It fires only on empty -> non-empty. Messages already waiting
 *     when you register do not trigger it, so drain after registering.
 *   - Only one process can be registered per queue (others get EBUSY).
 *   - If some process is blocked in mq_receive(), that receiver gets
 *     the message and no notification is sent.
 *
 * The parent registers for SIGUSR1 and waits with sigwaitinfo(). The
 * child opens the queue by name and sends three bursts of messages.
 */
#define _XOPEN_SOURCE 700

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"

#define QNAME   "/learn_mq_notify"
#define MSGSIZE 32

static void arm(mqd_t mq)
{
	struct sigevent sev = {
		.sigev_notify = SIGEV_SIGNAL,
		.sigev_signo = SIGUSR1,
	};

	if (mq_notify(mq, &sev))
		die("mq_notify");
}

static void child(void)
{
	static const char *const burst[][3] = {
		{ "a1", "a2", NULL },
		{ "b1", NULL },
		{ "quit", NULL },
	};
	size_t b, m;
	mqd_t mq;

	/* A separate process just opens the queue by its name. */
	mq = mq_open(QNAME, O_WRONLY);
	if (mq == (mqd_t)-1)
		die("child: mq_open");

	for (b = 0; b < 3; b++) {
		msleep(200);
		for (m = 0; burst[b][m]; m++) {
			if (mq_send(mq, burst[b][m], strlen(burst[b][m]) + 1, 0))
				die("child: mq_send");
			printf("  child  -> \"%s\"\n", burst[b][m]);
		}
	}
	mq_close(mq);
	exit(EXIT_SUCCESS);
}

int main(void)
{
	struct mq_attr attr = { .mq_maxmsg = 8, .mq_msgsize = MSGSIZE };
	char buf[MSGSIZE];
	siginfo_t si;
	sigset_t set;
	ssize_t n;
	pid_t pid;
	mqd_t mq;
	int done = 0;

	setvbuf(stdout, NULL, _IOLBF, 0);

	/*
	 * Block SIGUSR1 so it stays pending until sigwaitinfo() picks it
	 * up, instead of killing us (its default action).
	 */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &set, NULL))
		die("sigprocmask");

	unlink_stale(QNAME);
	/* Non-blocking, so draining stops with EAGAIN instead of hanging. */
	mq = mq_open(QNAME, O_CREAT | O_EXCL | O_RDONLY | O_NONBLOCK, 0600,
		     &attr);
	if (mq == (mqd_t)-1)
		die("mq_open");
	arm(mq);

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		child();

	while (!done) {
		if (sigwaitinfo(&set, &si) < 0) {
			if (errno == EINTR)
				continue;
			die("sigwaitinfo");
		}
		printf("  parent: SIGUSR1 (si_code %s, sender pid %s)\n",
		       si.si_code == SI_MESGQ ? "SI_MESGQ" : "other",
		       si.si_pid == pid ? "= child" : "?");

		arm(mq);	/* re-register FIRST, then drain */
		while ((n = mq_receive(mq, buf, sizeof(buf), NULL)) >= 0) {
			printf("  parent <- \"%s\"\n", buf);
			if (!strcmp(buf, "quit"))
				done = 1;
		}
		if (errno != EAGAIN)
			die("mq_receive");
	}

	if (waitpid(pid, NULL, 0) < 0)
		die("waitpid");
	mq_close(mq);		/* also drops the registration */
	mq_unlink(QNAME);
	return EXIT_SUCCESS;
}
