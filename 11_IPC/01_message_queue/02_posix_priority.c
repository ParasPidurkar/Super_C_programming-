// SPDX-License-Identifier: GPL-2.0
/*
 * 02_posix_priority - POSIX queues are priority queues, not plain FIFOs.
 *
 * mq_receive() always returns the oldest message of the highest priority.
 * Messages with equal priority come out in the order they were sent.
 */
#define _XOPEN_SOURCE 700

#include <fcntl.h>
#include <unistd.h>	/* sysconf() */

#include "common.h"

#define QNAME   "/learn_mq_priority"
#define MSGSIZE 32

static const struct {
	const char *text;
	unsigned int prio;	/* 0 (lowest) .. sysconf(_SC_MQ_PRIO_MAX) - 1 */
} msgs[] = {
	{ "low A",  1 },
	{ "high A", 10 },
	{ "mid",    5 },
	{ "low B",  1 },
	{ "high B", 10 },
	{ "zero",   0 },
};

#define NMSGS (sizeof(msgs) / sizeof(msgs[0]))

int main(void)
{
	struct mq_attr attr = { .mq_maxmsg = 8, .mq_msgsize = MSGSIZE };
	char buf[MSGSIZE];
	unsigned int prio;
	mqd_t mq;
	size_t i;

	unlink_stale(QNAME);
	mq = mq_open(QNAME, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
	if (mq == (mqd_t)-1)
		die("mq_open");

	printf("  priorities on this system: 0 .. %ld\n",
	       sysconf(_SC_MQ_PRIO_MAX) - 1);
	puts("  send order:");
	for (i = 0; i < NMSGS; i++) {
		if (mq_send(mq, msgs[i].text, strlen(msgs[i].text) + 1,
			    msgs[i].prio))
			die("mq_send");
		printf("    prio %2u  %s\n", msgs[i].prio, msgs[i].text);
	}
	print_attr(mq, "full");

	puts("  receive order:");
	for (i = 0; i < NMSGS; i++) {
		if (mq_receive(mq, buf, sizeof(buf), &prio) < 0)
			die("mq_receive");
		printf("    prio %2u  %s\n", prio, buf);
	}
	print_attr(mq, "drained");

	mq_close(mq);
	mq_unlink(QNAME);
	return EXIT_SUCCESS;
}
