// SPDX-License-Identifier: GPL-2.0
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "bst.h"

struct item {
	int key;
	struct bst_node node;
};

static int item_cmp(const struct bst_node *a, const struct bst_node *b)
{
	int x = bst_entry(a, struct item, node)->key;
	int y = bst_entry(b, struct item, node)->key;

	return (x > y) - (x < y);
}

static int item_key_cmp(const void *key, const struct bst_node *n)
{
	int x = *(const int *)key;
	int y = bst_entry(n, struct item, node)->key;

	return (x > y) - (x < y);
}

/*
 * Check every invariant without trusting bst_next(): each child points
 * back at its parent and every key lies strictly inside (lo, hi).
 * Return: number of nodes in the subtree.
 */
static size_t bst_check(const struct bst_node *n, const struct bst_node *parent,
			long lo, long hi)
{
	int key;

	if (!n)
		return 0;
	key = bst_entry(n, struct item, node)->key;
	assert(n->parent == parent);
	assert(key > lo && key < hi);
	return 1 + bst_check(n->left, n, lo, key) + bst_check(n->right, n, key, hi);
}

static void bst_verify(const struct bst_root *root, size_t count)
{
	const struct bst_node *n;
	size_t seen = 0;
	long prev = LONG_MIN;

	assert(bst_check(root->node, NULL, LONG_MIN, LONG_MAX) == count);
	assert(bst_empty(root) == !count);

	for (n = bst_first(root); n; n = bst_next(n), seen++) {
		int key = bst_entry(n, struct item, node)->key;

		assert(key > prev);
		prev = key;
	}
	assert(seen == count);

	for (n = bst_last(root); n; n = bst_prev(n))
		seen--;
	assert(seen == 0);
}

/* Small deterministic PRNG so failures reproduce. */
static unsigned int rnd(unsigned int *state)
{
	*state = *state * 1103515245u + 12345u;
	return *state >> 16;
}

static void shuffle(int *a, unsigned int n, unsigned int *state)
{
	unsigned int i, j;
	int t;

	for (i = n - 1; i > 0; i--) {
		j = rnd(state) % (i + 1);
		t = a[i];
		a[i] = a[j];
		a[j] = t;
	}
}

#define BST_N 500

static void test_bst(void)
{
	static struct item items[BST_N];
	static int order[BST_N];
	struct bst_root root = BST_ROOT;
	struct item dup;
	unsigned int seed = 1;
	int i, key;

	assert(bst_empty(&root));
	assert(!bst_first(&root) && !bst_last(&root));
	key = 0;
	assert(!bst_find(&root, &key, item_key_cmp));

	/* Random insert order, keys 0, 2, 4, ... so odd keys are misses. */
	for (i = 0; i < BST_N; i++)
		order[i] = i;
	shuffle(order, BST_N, &seed);
	for (i = 0; i < BST_N; i++) {
		items[order[i]].key = 2 * order[i];
		assert(!bst_add(&root, &items[order[i]].node, item_cmp));
	}
	bst_verify(&root, BST_N);

	dup.key = 42;
	assert(bst_add(&root, &dup.node, item_cmp) == &items[21].node);
	bst_verify(&root, BST_N);

	for (i = 0; i < 2 * BST_N; i++) {
		struct bst_node *n = bst_find(&root, &i, item_key_cmp);

		assert(i % 2 ? !n : n == &items[i / 2].node);
	}

	/* Erase in a different random order, checking the tree every time. */
	shuffle(order, BST_N, &seed);
	for (i = 0; i < BST_N; i++) {
		bst_erase(&root, &items[order[i]].node);
		key = items[order[i]].key;
		assert(!bst_find(&root, &key, item_key_cmp));
		bst_verify(&root, (size_t)(BST_N - i - 1));
	}

	/* Sorted insert: degenerate, list-shaped tree. Nothing may recurse. */
	for (i = 0; i < BST_N; i++)
		assert(!bst_add(&root, &items[i].node, item_cmp));
	bst_verify(&root, BST_N);
	for (i = 0; i < BST_N; i++)
		bst_erase(&root, bst_first(&root));
	assert(bst_empty(&root));

	puts("bst: ok");
}

int main(void)
{
	test_bst();
	return EXIT_SUCCESS;
}
