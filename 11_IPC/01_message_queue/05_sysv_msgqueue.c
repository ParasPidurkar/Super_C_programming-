// SPDX-License-Identifier: GPL-2.0
/*
 * 05_sysv_msgqueue - the older System V API: msgget/msgsnd/msgrcv/msgctl.
 *
 * Instead of a priority, every message carries a positive "type" (mtype),
 * and the receiver chooses which messages it wants:
 *
 *   msgtyp == 0   the oldest message of any type (plain FIFO)
 *   msgtyp  > 0   the oldest message of exactly that type
 *   msgtyp  < 0   the oldest message with the lowest type <= |msgtyp|
 *
 * So one queue can carry several independent "channels", for example one
 * type per client process.
 */
#define _XOPEN_SOURCE 700

#include <sys/ipc.h>
#include <sys/msg.h>

#include "common.h"

#define TEXTSIZE 32

/* The layout msgsnd()/msgrcv() expect: a long type, then the payload. */
struct message {
	long mtype;		/* must be > 0 */
	char mtext[TEXTSIZE];
};

static void send(int id, long type, const char *text)
{
	struct message m = { .mtype = type };

	snprintf(m.mtext, sizeof(m.mtext), "%s", text);
	/* The size counts only the payload, NOT the mtype field. */
	if (msgsnd(id, &m, strlen(m.mtext) + 1, 0))
		die("msgsnd");
	printf("  sent  type %ld  \"%s\"\n", type, text);
}

/* Return: 0 if a message was received, -1 if none matched (ENOMSG). */
static int recv(int id, long msgtyp)
{
	struct message m;
	ssize_t n;

	n = msgrcv(id, &m, sizeof(m.mtext), msgtyp, IPC_NOWAIT);
	if (n < 0) {
		if (errno != ENOMSG)
			die("msgrcv");
		printf("  msgrcv(msgtyp=%2ld) -> ENOMSG (nothing matches)\n",
		       msgtyp);
		return -1;
	}
	printf("  msgrcv(msgtyp=%2ld) -> type %ld  \"%s\"\n", msgtyp, m.mtype,
	       m.mtext);
	return 0;
}

static void stat_queue(int id)
{
	struct msqid_ds ds;

	if (msgctl(id, IPC_STAT, &ds))
		die("msgctl(IPC_STAT)");
	printf("  [IPC_STAT] messages=%lu max bytes=%lu\n",
	       (unsigned long)ds.msg_qnum, (unsigned long)ds.msg_qbytes);
}

int main(void)
{
	struct message m;
	ssize_t n;
	int id;

	/*
	 * IPC_PRIVATE always makes a new queue, so stale queues can't get in
	 * the way. Unrelated processes would instead agree on a key, usually
	 * from ftok("/some/path", 'A').
	 */
	id = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
	if (id < 0)
		die("msgget");
	printf("  queue id %d (see it with: ipcs -q)\n", id);

	puts("1. send six messages of three types");
	send(id, 3, "log: disk ok");
	send(id, 1, "urgent: overheat");
	send(id, 2, "job: build");
	send(id, 1, "urgent: fan");
	send(id, 3, "log: idle");
	send(id, 2, "job: test");
	stat_queue(id);

	puts("2. msgtyp > 0: only type 2");
	recv(id, 2);

	puts("3. msgtyp < 0: lowest type <= 2 first, until none are left");
	while (!recv(id, -2))
		;

	puts("4. msgtyp = 0: whatever is oldest");
	while (!recv(id, 0))
		;

	puts("5. a buffer that is too small");
	send(id, 1, "a rather long message");
	n = msgrcv(id, &m, 8, 0, IPC_NOWAIT);
	if (n < 0 && errno == E2BIG)
		puts("  msgrcv(size 8)              -> E2BIG, message stays queued");
	else
		die("msgrcv expected E2BIG");
	/* MSG_NOERROR truncates instead; the cut-off bytes are lost. */
	n = msgrcv(id, &m, 8, 0, IPC_NOWAIT | MSG_NOERROR);
	if (n < 0)
		die("msgrcv");
	printf("  msgrcv(size 8, MSG_NOERROR) -> %zd bytes: \"%.*s\"\n", n,
	       (int)n, m.mtext);

	/* Like mq_unlink(): without this the queue outlives the process. */
	if (msgctl(id, IPC_RMID, NULL))
		die("msgctl(IPC_RMID)");
	puts("  queue removed (IPC_RMID)");
	return EXIT_SUCCESS;
}
