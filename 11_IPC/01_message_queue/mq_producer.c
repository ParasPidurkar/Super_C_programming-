// SPDX-License-Identifier: GPL-2.0
/*
 * mq_producer - send each line you type to a POSIX message queue.
 *
 * Usage: ./mq_producer          (run ./mq_consumer in another terminal)
 *
 *   hello        sent with priority 0
 *   !fire        sent with priority 9, so it jumps ahead of waiting messages
 *   quit         tells the consumer to exit
 *   Ctrl-D       exits the producer; the queue and its messages stay
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
	char line[CHAT_MSGSIZE], *text;
	unsigned int prio;
	mqd_t mq;

	/* No O_EXCL: whichever program starts first creates the queue. */
	mq = mq_open(CHAT_NAME, O_CREAT | O_WRONLY, 0600, &attr);
	if (mq == (mqd_t)-1)
		die("mq_open");
	printf("Writing to %s. Type lines; '!text' is urgent, Ctrl-D exits.\n",
	       CHAT_NAME);

	while (fgets(line, sizeof(line), stdin)) {
		line[strcspn(line, "\n")] = '\0';
		text = line;
		prio = 0;
		if (text[0] == '!') {
			text++;
			prio = 9;
		}
		/* Blocks if the consumer has fallen CHAT_MAXMSG messages behind. */
		if (mq_send(mq, text, strlen(text) + 1, prio))
			die("mq_send");
		printf("  sent (prio %u)\n", prio);
	}

	mq_close(mq);	/* note: no mq_unlink, the consumer owns the queue */
	return EXIT_SUCCESS;
}
