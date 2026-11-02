/* SPDX-License-Identifier: GPL-2.0 */
/*
 * common.h - small helpers shared by the message queue examples.
 *
 * Every .c file defines _XOPEN_SOURCE before including anything, so the
 * POSIX and System V declarations used here are visible under -std=c11.
 */
#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Names shared by mq_producer and mq_consumer. */
#define CHAT_NAME    "/learn_mq_chat"
#define CHAT_MAXMSG  8
#define CHAT_MSGSIZE 128

/* Print what failed and why (from errno), then exit. */
_Noreturn static inline void die(const char *what)
{
	perror(what);
	exit(EXIT_FAILURE);
}

/*
 * Remove a queue left behind by an earlier run that crashed. POSIX
 * queues live in the kernel until unlinked, not until the process exits.
 */
static inline void unlink_stale(const char *name)
{
	if (mq_unlink(name) && errno != ENOENT)
		die("mq_unlink");
}

static inline void print_attr(mqd_t mq, const char *label)
{
	struct mq_attr a;

	if (mq_getattr(mq, &a))
		die("mq_getattr");
	printf("  [%s] maxmsg=%ld msgsize=%ld curmsgs=%ld flags=%s\n", label,
	       a.mq_maxmsg, a.mq_msgsize, a.mq_curmsgs,
	       (a.mq_flags & O_NONBLOCK) ? "O_NONBLOCK" : "0");
}

static inline void msleep(long ms)
{
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };

	while (nanosleep(&ts, &ts) && errno == EINTR)
		;
}

#endif /* COMMON_H */
