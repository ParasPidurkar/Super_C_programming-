// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "aqueue.h"

static unsigned int roundup_pow_of_two(unsigned int n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

static void *aqueue_slot(const struct aqueue *q, unsigned int idx)
{
	return (char *)q->data + (size_t)(idx & q->mask) * q->esize;
}

/**
 * aqueue_init - allocate a ring buffer
 * @q:     queue to initialise
 * @size:  minimum number of elements; rounded up to a power of two
 * @esize: size of one element in bytes
 *
 * Return: 0 on success, -EINVAL on bad arguments, -ENOMEM on allocation
 * failure.
 */
int aqueue_init(struct aqueue *q, unsigned int size, size_t esize)
{
	if (!size || !esize || size > (UINT_MAX / 2) + 1)
		return -EINVAL;

	size = roundup_pow_of_two(size);

	q->data = calloc(size, esize);
	if (!q->data)
		return -ENOMEM;

	q->in = 0;
	q->out = 0;
	q->mask = size - 1;
	q->esize = esize;
	return 0;
}

void aqueue_free(struct aqueue *q)
{
	free(q->data);
	q->data = NULL;
	q->in = 0;
	q->out = 0;
	q->mask = 0;
}

/* Return: 0 on success, -ENOSPC if the queue is full. */
int aqueue_push(struct aqueue *q, const void *elem)
{
	if (aqueue_full(q))
		return -ENOSPC;

	memcpy(aqueue_slot(q, q->in), elem, q->esize);
	q->in++;
	return 0;
}

/*
 * Copy the oldest element into @elem without removing it.
 * Return: 0 on success, -ENODATA if the queue is empty.
 */
int aqueue_peek(const struct aqueue *q, void *elem)
{
	if (aqueue_empty(q))
		return -ENODATA;

	memcpy(elem, aqueue_slot(q, q->out), q->esize);
	return 0;
}

/*
 * Remove the oldest element, copying it into @elem unless @elem is NULL.
 * Return: 0 on success, -ENODATA if the queue is empty.
 */
int aqueue_pop(struct aqueue *q, void *elem)
{
	if (aqueue_empty(q))
		return -ENODATA;

	if (elem)
		memcpy(elem, aqueue_slot(q, q->out), q->esize);
	q->out++;
	return 0;
}
