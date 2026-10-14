// SPDX-License-Identifier: GPL-2.0
/*
 * demo - interactive playground for both queues.
 *
 * Usage: ./demo [aqueue-size]     (default size 4)
 * Type 'help' at the prompt for commands.
 */
#define _POSIX_C_SOURCE 200809L	/* isatty() under -std=c11 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aqueue.h"
#include "lqueue.h"

struct job {
	int id;
	char name[32];
	struct lqueue_node node;
};

static struct aqueue aq;
static struct lqueue lq = LQUEUE_INIT(lq);
static int next_job_id = 1;
static int use_lqueue;	/* which queue push/pop/peek act on */

/* Print in/out/len and the backing array; '.' marks a slot not in use. */
static void aqueue_show(void)
{
	const int *slots = aq.data;
	unsigned int i, len = aqueue_len(&aq);

	printf("  in=%u out=%u len=%u/%u  [", aq.in, aq.out, len,
	       aqueue_size(&aq));
	for (i = 0; i < aqueue_size(&aq); i++) {
		/* Slot i is live if it lies within 'len' slots after 'out'. */
		if (((i - aq.out) & aq.mask) < len)
			printf(" %d", slots[i]);
		else
			printf(" .");
	}
	printf(" ]  head->slot %u, next push->slot %u\n", aq.out & aq.mask,
	       aq.in & aq.mask);
}

static void lqueue_show(void)
{
	const struct lqueue_node *n;

	printf("  len=%zu  head", lqueue_len(&lq));
	for (n = lq.head; n; n = n->next) {
		const struct job *j = lqueue_entry(n, struct job, node);

		printf(" -> [%d %s]", j->id, j->name);
	}
	printf(" -> NULL   tail %s\n",
	       lq.tail == &lq.head ? "= &head (empty)" : "= &last->next");
}

static void show(void)
{
	if (use_lqueue)
		lqueue_show();
	else
		aqueue_show();
}

static void help(void)
{
	puts("Commands:\n"
	     "  push <x>   aqueue: push integer x   lqueue: push a job named x\n"
	     "  pop        remove the oldest item\n"
	     "  peek       look at the oldest item without removing it\n"
	     "  show       print the queue state\n"
	     "  switch     toggle between aqueue and lqueue\n"
	     "  wrap       (aqueue) jump in/out to just below UINT_MAX\n"
	     "  help       this text\n"
	     "  quit       exit (Ctrl-D works too)\n"
	     "Several pushes at once: push 1 2 3");
}

static void do_push(char *args)
{
	char *tok, *end;
	long val;
	int v, ret;

	tok = strtok(args, " \t");
	if (!tok) {
		puts("  usage: push <x> [x...]");
		return;
	}

	for (; tok; tok = strtok(NULL, " \t")) {
		if (use_lqueue) {
			struct job *j = malloc(sizeof(*j));

			if (!j) {
				perror("malloc");
				return;
			}
			j->id = next_job_id++;
			snprintf(j->name, sizeof(j->name), "%s", tok);
			lqueue_push(&lq, &j->node);
			printf("  pushed job %d '%s'\n", j->id, j->name);
			continue;
		}

		errno = 0;
		val = strtol(tok, &end, 10);
		if (*end || errno || val < INT_MIN || val > INT_MAX) {
			printf("  '%s' is not an integer\n", tok);
			return;
		}
		v = (int)val;
		ret = aqueue_push(&aq, &v);
		if (ret == -ENOSPC) {
			printf("  push %d -> -ENOSPC: queue is full\n", v);
			return;
		}
		printf("  pushed %d into slot %u\n", v, (aq.in - 1) & aq.mask);
	}
}

static void do_pop(int remove)
{
	const char *verb = remove ? "popped" : "peeked";

	if (use_lqueue) {
		struct lqueue_node *n = remove ? lqueue_pop(&lq) : lqueue_peek(&lq);
		struct job *j;

		if (!n) {
			printf("  %s NULL: queue is empty\n", remove ? "pop" : "peek");
			return;
		}
		/* container_of: node pointer -> the struct job around it */
		j = lqueue_entry(n, struct job, node);
		printf("  %s job %d '%s'\n", verb, j->id, j->name);
		if (remove)
			free(j);
		return;
	}

	{
		unsigned int slot = aq.out & aq.mask;
		int v, ret;

		ret = remove ? aqueue_pop(&aq, &v) : aqueue_peek(&aq, &v);
		if (ret == -ENODATA) {
			printf("  %s -> -ENODATA: queue is empty\n",
			       remove ? "pop" : "peek");
			return;
		}
		printf("  %s %d from slot %u\n", verb, v, slot);
	}
}

static void do_wrap(void)
{
	if (use_lqueue) {
		puts("  wrap only applies to aqueue");
		return;
	}
	if (!aqueue_empty(&aq)) {
		puts("  pop everything first (wrap moves both counters)");
		return;
	}
	aq.in = aq.out = UINT_MAX - 1;
	puts("  in/out set to UINT_MAX - 1; push a few to watch 'in' wrap to 0");
}

int main(int argc, char **argv)
{
	unsigned long size = 4;
	char line[256], *cmd, *args;
	struct lqueue_node *n;
	int ret;

	if (argc > 1) {
		char *end;

		size = strtoul(argv[1], &end, 10);
		if (*end || size > UINT_MAX) {
			fprintf(stderr, "usage: %s [aqueue-size]\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

	ret = aqueue_init(&aq, (unsigned int)size, sizeof(int));
	if (ret) {
		fprintf(stderr, "aqueue_init(%lu): %s\n", size, strerror(-ret));
		return EXIT_FAILURE;
	}

	printf("aqueue capacity %u (asked for %lu, rounded up to a power of two)\n",
	       aqueue_size(&aq), size);
	puts("Type 'help' for commands.");
	show();

	for (;;) {
		printf("%s> ", use_lqueue ? "lqueue" : "aqueue");
		fflush(stdout);
		if (!fgets(line, sizeof(line), stdin)) {
			putchar('\n');
			break;
		}
		line[strcspn(line, "\n")] = '\0';

		cmd = strtok(line, " \t");
		if (!cmd)
			continue;
		args = strtok(NULL, "");
		if (!args)
			args = "";

		if (!strcmp(cmd, "push")) {
			do_push(args);
		} else if (!strcmp(cmd, "pop")) {
			do_pop(1);
		} else if (!strcmp(cmd, "peek")) {
			do_pop(0);
		} else if (!strcmp(cmd, "show")) {
			/* shown below */
		} else if (!strcmp(cmd, "switch")) {
			use_lqueue = !use_lqueue;
			printf("  now using %s\n", use_lqueue ? "lqueue" : "aqueue");
		} else if (!strcmp(cmd, "wrap")) {
			do_wrap();
		} else if (!strcmp(cmd, "help")) {
			help();
			continue;
		} else if (!strcmp(cmd, "quit") || !strcmp(cmd, "exit")) {
			/* A terminal already echoed the Enter; piped input did not. */
			if (!isatty(STDIN_FILENO))
				putchar('\n');
			break;
		} else {
			printf("  unknown command '%s' (try 'help')\n", cmd);
			continue;
		}
		show();
	}

	while ((n = lqueue_pop(&lq)))
		free(lqueue_entry(n, struct job, node));
	aqueue_free(&aq);
	return EXIT_SUCCESS;
}
