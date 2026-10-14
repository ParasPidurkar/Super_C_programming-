// SPDX-License-Identifier: GPL-2.0
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "aqueue.h"
#include "lqueue.h"

struct job {
	int id;
	struct lqueue_node node;
};

static void test_lqueue(void)
{
	struct lqueue q = LQUEUE_INIT(q);
	struct job jobs[5];
	struct lqueue_node *n;
	int i;

	assert(lqueue_empty(&q));
	assert(!lqueue_pop(&q));

	for (i = 0; i < 5; i++) {
		jobs[i].id = i;
		lqueue_push(&q, &jobs[i].node);
	}
	assert(lqueue_len(&q) == 5);
	assert(lqueue_entry(lqueue_peek(&q), struct job, node)->id == 0);

	for (i = 0; i < 5; i++) {
		n = lqueue_pop(&q);
		assert(lqueue_entry(n, struct job, node)->id == i);
	}
	assert(lqueue_empty(&q));
	assert(lqueue_len(&q) == 0);

	/* tail must have been reset to &q.head, or this push is lost */
	lqueue_push(&q, &jobs[3].node);
	assert(lqueue_entry(lqueue_pop(&q), struct job, node)->id == 3);
	assert(!lqueue_pop(&q));

	puts("lqueue: ok");
}

static void test_aqueue(void)
{
	struct aqueue q;
	int i, v;

	assert(aqueue_init(&q, 0, sizeof(int)) == -EINVAL);
	assert(aqueue_init(&q, 4, 0) == -EINVAL);

	assert(aqueue_init(&q, 5, sizeof(int)) == 0);
	assert(aqueue_size(&q) == 8);
	assert(aqueue_pop(&q, &v) == -ENODATA);
	assert(aqueue_peek(&q, &v) == -ENODATA);

	for (i = 0; i < 8; i++)
		assert(aqueue_push(&q, &i) == 0);
	assert(aqueue_full(&q));
	assert(aqueue_push(&q, &i) == -ENOSPC);

	assert(aqueue_peek(&q, &v) == 0 && v == 0);
	for (i = 0; i < 8; i++) {
		assert(aqueue_pop(&q, &v) == 0);
		assert(v == i);
	}
	assert(aqueue_empty(&q));

	/* Force the free-running counters across UINT_MAX. */
	q.in = q.out = UINT_MAX - 2;
	for (i = 0; i < 8; i++)
		assert(aqueue_push(&q, &i) == 0);
	assert(aqueue_full(&q) && aqueue_len(&q) == 8);
	for (i = 0; i < 8; i++) {
		assert(aqueue_pop(&q, &v) == 0);
		assert(v == i);
	}
	assert(aqueue_empty(&q));

	aqueue_free(&q);
	puts("aqueue: ok");
}

int main(void)
{
	test_lqueue();
	test_aqueue();
	return EXIT_SUCCESS;
}
