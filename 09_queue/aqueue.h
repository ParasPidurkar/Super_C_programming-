/* SPDX-License-Identifier: GPL-2.0 */
/*
 * aqueue - fixed size, array backed FIFO ring buffer.
 *
 * Capacity is rounded up to a power of two so that indexing is a mask,
 * not a modulo. 'in' and 'out' are free-running counters: they are never
 * reset and are allowed to wrap. Unsigned arithmetic makes 'in - out' the
 * element count at all times, so there is no wasted slot and no separate
 * "full" flag.
 */
#ifndef AQUEUE_H
#define AQUEUE_H

#include <stdbool.h>
#include <stddef.h>

struct aqueue {
	unsigned int in;
	unsigned int out;
	unsigned int mask;
	size_t esize;
	void *data;
};

int aqueue_init(struct aqueue *q, unsigned int size, size_t esize);
void aqueue_free(struct aqueue *q);

int aqueue_push(struct aqueue *q, const void *elem);
int aqueue_pop(struct aqueue *q, void *elem);
int aqueue_peek(const struct aqueue *q, void *elem);

static inline unsigned int aqueue_size(const struct aqueue *q)
{
	return q->mask + 1;
}

static inline unsigned int aqueue_len(const struct aqueue *q)
{
	return q->in - q->out;
}

static inline bool aqueue_empty(const struct aqueue *q)
{
	return q->in == q->out;
}

static inline bool aqueue_full(const struct aqueue *q)
{
	return aqueue_len(q) > q->mask;
}

#endif /* AQUEUE_H */
