/* SPDX-License-Identifier: GPL-2.0 */
/*
 * lqueue - intrusive singly linked FIFO queue.
 *
 * The queue never allocates. Embed a struct lqueue_node in your own
 * structure and use container_of() to get back to it, exactly like the
 * kernel's list_head.
 *
 * 'tail' points at the 'next' field of the last node, or at 'head' when
 * the queue is empty. That removes the "is the queue empty?" special case
 * from push entirely.
 *
 * A struct lqueue must not be copied by value: 'tail' may point into it.
 */
#ifndef LQUEUE_H
#define LQUEUE_H

#include <stdbool.h>
#include <stddef.h>

#ifndef container_of
#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#endif

struct lqueue_node {
	struct lqueue_node *next;
};

struct lqueue {
	struct lqueue_node *head;
	struct lqueue_node **tail;
	size_t len;
};

#define LQUEUE_INIT(name) { .head = NULL, .tail = &(name).head, .len = 0 }

static inline void lqueue_init(struct lqueue *q)
{
	q->head = NULL;
	q->tail = &q->head;
	q->len = 0;
}

static inline bool lqueue_empty(const struct lqueue *q)
{
	return !q->head;
}

static inline size_t lqueue_len(const struct lqueue *q)
{
	return q->len;
}

static inline void lqueue_push(struct lqueue *q, struct lqueue_node *n)
{
	n->next = NULL;
	*q->tail = n;
	q->tail = &n->next;
	q->len++;
}

static inline struct lqueue_node *lqueue_peek(const struct lqueue *q)
{
	return q->head;
}

static inline struct lqueue_node *lqueue_pop(struct lqueue *q)
{
	struct lqueue_node *n = q->head;

	if (!n)
		return NULL;

	q->head = n->next;
	if (!q->head)
		q->tail = &q->head;
	q->len--;
	return n;
}

#define lqueue_entry(ptr, type, member) container_of(ptr, type, member)

#endif /* LQUEUE_H */
