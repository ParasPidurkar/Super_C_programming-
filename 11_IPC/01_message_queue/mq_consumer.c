// SPDX-License-Identifier: GPL-2.0
/*
 * mq_consumer - print every message from the queue mq_producer writes to.
 *
 * Usage: ./mq_consumer          (exits and removes the queue on "quit")
 */
#define _XOPEN_SOURCE 700

#include <fcntl.h>

#include "common.h"

int main(void)
{
	struct mq_attr attr = {
		.mq_maxmsg = CHAT_MAXMSG,
		.mq_msgsize = CHAT_MSGSIZE,
	};
	unsigned int prio;
	char *buf;
	ssize_t n;
	mqd_t mq;

	mq = mq_open(CHAT_NAME, O_CREAT | O_RDONLY, 0600, &attr);
	if (mq == (mqd_t)-1)
		die("mq_open");

	/*
	 * If the queue already existed, O_CREAT ignored our attr. Ask the
	 * queue for its real mq_msgsize and size the buffer from that.
	 */
	if (mq_getattr(mq, &attr))
		die("mq_getattr");
	buf = malloc((size_t)attr.mq_msgsize);
	if (!buf)
		die("malloc");
	printf("Reading from %s (%ld already waiting). Waiting for messages...\n",
	       CHAT_NAME, attr.mq_curmsgs);

	for (;;) {
		n = mq_receive(mq, buf, (size_t)attr.mq_msgsize, &prio);
		if (n < 0)
			die("mq_receive");
		printf("  [prio %u] %s\n", prio, buf);
		fflush(stdout);
		if (!strcmp(buf, "quit"))
			break;
	}

	free(buf);
	mq_close(mq);
	mq_unlink(CHAT_NAME);
	puts("Queue removed.");
	return EXIT_SUCCESS;
}
