/* SPDX-License-Identifier: GPL-2.0 */
/*
 * bst - intrusive binary search tree.
 *
 * The tree never allocates. Embed a struct bst_node in your
 * own structure and use bst_entry() to get back to it.
 *
 * The tree is ordered by a caller-supplied compare function, so it knows
 * nothing about keys. Duplicates are rejected: bst_add() hands back the
 * node already in the tree instead.
 *
 * Every node keeps a parent pointer. That buys three things: erase needs
 * no compare function, in-order iteration needs no stack, and nothing in
 * here recurses, so a degenerate (list-shaped) tree cannot blow the stack.
 *
 * The tree is NOT balanced. Inserting sorted keys gives O(n) depth. For
 * guaranteed O(log n), use a red-black tree (the kernel's rbtree); its
 * node layout and API are this one plus a colour bit and rotations.
 */
#ifndef BST_H
#define BST_H

#include <stdbool.h>
#include <stddef.h>

#ifndef container_of
#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#endif

struct bst_node {
	struct bst_node *parent;
	struct bst_node *left;
	struct bst_node *right;
};

struct bst_root {
	struct bst_node *node;
};

#define BST_ROOT { NULL }

#define bst_entry(ptr, type, member) container_of(ptr, type, member)

static inline bool bst_empty(const struct bst_root *root)
{
	return !root->node;
}

/* Order two nodes: <0, 0, >0 like strcmp(). Used by bst_add(). */
typedef int (*bst_cmp_t)(const struct bst_node *a, const struct bst_node *b);
/* Order a search key against a node. Used by bst_find(). */
typedef int (*bst_key_cmp_t)(const void *key, const struct bst_node *n);

struct bst_node *bst_add(struct bst_root *root, struct bst_node *node,
			 bst_cmp_t cmp);
struct bst_node *bst_find(const struct bst_root *root, const void *key,
			  bst_key_cmp_t cmp);
void bst_erase(struct bst_root *root, struct bst_node *node);

struct bst_node *bst_first(const struct bst_root *root);
struct bst_node *bst_last(const struct bst_root *root);
struct bst_node *bst_next(const struct bst_node *node);
struct bst_node *bst_prev(const struct bst_node *node);

#endif /* BST_H */
