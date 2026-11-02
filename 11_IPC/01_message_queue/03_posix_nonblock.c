// SPDX-License-Identifier: GPL-2.0
/*
 * 03_posix_nonblock - the error cases you will hit in real code.
 *
 *   1. O_NONBLOCK + full queue   -> mq_send()    fails with EAGAIN
 *   2. message > mq_msgsize      -> mq_send()    fails with EMSGSIZE
 *   3. buffer  < mq_msgsize      -> mq_receive() fails with EMSGSIZE
 *   4. O_NONBLOCK + empty queue  -> mq_receive() fails with EAGAIN
 *   5. blocking + deadline       -> mq_timedreceive() fails with ETIMEDOUT
 */
#define _XOPEN_SOURCE 700

#include <fcntl.h>

#include "common.h"

#define QNAME   "/learn_mq_nonblock"
#define MSGSIZE 32

static void expect(int ok, int err, int want, const char *what)
{
	if (ok)
		printf("  %-34s unexpectedly succeeded\n", what);
	else if (err == want)
		printf("  %-34s -> %s\n", what, strerror(err));
	else {
		errno = err;
		die(what);
	}
}

int main(void)
{
	struct mq_attr attr = { .mq_maxmsg = 2, .mq_msgsize = MSGSIZE };
	struct mq_attr blocking = { .mq_flags = 0 };
	char buf[MSGSIZE], small[8], big[MSGSIZE + 1], text[16];
	struct timespec deadline;
	ssize_t n;
	mqd_t mq;
	int i;

	unlink_stale(QNAME);
	mq = mq_open(QNAME, O_CREAT | O_EXCL | O_RDWR | O_NONBLOCK, 0600,
		     &attr);
	if (mq == (mqd_t)-1)
		die("mq_open");
	print_attr(mq, "created");

	puts("1. fill a 2-slot queue without blocking");
	for (i = 1;; i++) {
		snprintf(text, sizeof(text), "msg %d", i);
		if (!mq_send(mq, text, strlen(text) + 1, 0)) {
			printf("  sent \"%s\"\n", text);
			continue;
		}
		expect(0, errno, EAGAIN, "mq_send on a full queue");
		break;
	}

	puts("2. send a message larger than mq_msgsize");
	memset(big, 'x', sizeof(big));
	/*
	 * Call first, then read errno. Inside one argument list the order
	 * of evaluation is unspecified, so errno could be read too early.
	 */
	i = mq_send(mq, big, sizeof(big), 0);
	expect(!i, errno, EMSGSIZE, "mq_send of 33 bytes (msgsize 32)");

	puts("3. receive into a buffer smaller than mq_msgsize");
	/* Fails even though the waiting message is only 6 bytes long. */
	n = mq_receive(mq, small, sizeof(small), NULL);
	expect(n >= 0, errno, EMSGSIZE, "mq_receive with 8-byte buffer");

	puts("4. drain, then receive from an empty queue");
	while ((n = mq_receive(mq, buf, sizeof(buf), NULL)) >= 0)
		printf("  got \"%s\"\n", buf);
	expect(0, errno, EAGAIN, "mq_receive on an empty queue");

	puts("5. switch to blocking mode and wait at most 500 ms");
	/* mq_setattr() only looks at mq_flags; everything else is fixed. */
	if (mq_setattr(mq, &blocking, NULL))
		die("mq_setattr");
	print_attr(mq, "now blocking");

	/* The timeout is an absolute CLOCK_REALTIME time, not a duration. */
	clock_gettime(CLOCK_REALTIME, &deadline);
	deadline.tv_nsec += 500000000L;
	if (deadline.tv_nsec >= 1000000000L) {
		deadline.tv_sec++;
		deadline.tv_nsec -= 1000000000L;
	}
	n = mq_timedreceive(mq, buf, sizeof(buf), NULL, &deadline);
	expect(n >= 0, errno, ETIMEDOUT, "mq_timedreceive on an empty queue");

	mq_close(mq);
	mq_unlink(QNAME);
	return EXIT_SUCCESS;
}
