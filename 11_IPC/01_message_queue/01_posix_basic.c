// SPDX-License-Identifier: GPL-2.0
/*
 * 01_posix_basic - the whole POSIX message queue lifecycle in one program.
 *
 *   mq_open(O_CREAT) -> fork -> mq_send / mq_receive -> mq_close -> mq_unlink
 *
 * The parent creates a queue that holds at most 4 messages, then forks.
 * The child receives slowly; the parent sends 6 messages and then "quit".
 * Once 4 messages are waiting, mq_send() blocks until the child makes
 * room, so the queue also acts as flow control between the two processes.
 */
#define _XOPEN_SOURCE 700

#include <fcntl.h>	/* O_CREAT, O_EXCL, O_RDWR */
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"

#define QNAME   "/learn_mq_basic"	/* must start with '/' */
#define MAXMSG  4
#define MSGSIZE 64

static void child(mqd_t mq)
{
	/* The buffer must be at least mq_msgsize, or mq_receive() fails. */
	char buf[MSGSIZE];
	unsigned int prio;
	ssize_t n;

	for (;;) {
		n = mq_receive(mq, buf, sizeof(buf), &prio);
		if (n < 0)
			die("child: mq_receive");
		printf("  child  <- \"%s\" (%zd bytes)\n", buf, n);
		if (!strcmp(buf, "quit"))
			break;
		msleep(100);	/* be slow so the queue fills up */
	}
	mq_close(mq);
	exit(EXIT_SUCCESS);
}

int main(void)
{
	struct mq_attr attr = { .mq_maxmsg = MAXMSG, .mq_msgsize = MSGSIZE };
	char text[MSGSIZE];
	mqd_t mq;
	pid_t pid;
	int i;

	/* Line-buffer stdout so both processes' lines appear as they happen. */
	setvbuf(stdout, NULL, _IOLBF, 0);

	unlink_stale(QNAME);
	/* O_EXCL: fail instead of silently reusing someone else's queue. */
	mq = mq_open(QNAME, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
	if (mq == (mqd_t)-1)
		die("mq_open");
	print_attr(mq, "created");

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		child(mq);	/* the descriptor is inherited across fork */

	for (i = 1; i <= 6; i++) {
		snprintf(text, sizeof(text), "message %d", i);
		/* Messages are bytes, not strings: send the NUL too. */
		if (mq_send(mq, text, strlen(text) + 1, 0))
			die("mq_send");
		printf("  parent -> \"%s\"\n", text);
	}
	if (mq_send(mq, "quit", 5, 0))
		die("mq_send");

	if (waitpid(pid, NULL, 0) < 0)
		die("waitpid");

	print_attr(mq, "after child exits");
	mq_close(mq);		/* drop our descriptor */
	mq_unlink(QNAME);	/* remove the queue itself */
	puts("  queue closed and unlinked");
	return EXIT_SUCCESS;
}
