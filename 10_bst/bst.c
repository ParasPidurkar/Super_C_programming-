// SPDX-License-Identifier: GPL-2.0
#include "bst.h"

/*
 * Return the pointer that points at @node: its parent's left or right
 * field, or the root itself. Writing through it replaces @node in the
 * tree without caring which of the three cases it was.
 */
static struct bst_node **bst_link(struct bst_root *root, struct bst_node *node)
{
	struct bst_node *parent = node->parent;

	if (!parent)
		return &root->node;
	return parent->left == node ? &parent->left : &parent->right;
}

/**
 * bst_add - insert a node
 * @root: tree
 * @node: node to insert; its fields need not be initialised
 * @cmp:  ordering of two nodes
 *
 * Return: NULL if @node was inserted, or the node already in the tree
 * that compares equal to @node (which is then left untouched).
 */
struct bst_node *bst_add(struct bst_root *root, struct bst_node *node,
			 bst_cmp_t cmp)
{
	struct bst_node **link = &root->node, *parent = NULL;

	while (*link) {
		int c = cmp(node, *link);

		if (!c)
			return *link;
		parent = *link;
		link = c < 0 ? &parent->left : &parent->right;
	}

	node->parent = parent;
	node->left = NULL;
	node->right = NULL;
	*link = node;
	return NULL;
}

/* Return: the node comparing equal to @key, or NULL. */
struct bst_node *bst_find(const struct bst_root *root, const void *key,
			  bst_key_cmp_t cmp)
{
	struct bst_node *node = root->node;

	while (node) {
		int c = cmp(key, node);

		if (!c)
			return node;
		node = c < 0 ? node->left : node->right;
	}
	return NULL;
}

/**
 * bst_erase - remove a node from the tree
 * @root: tree
 * @node: node currently in @root
 *
 * Nodes are relinked, never copied: every other node stays at the same
 * address, so pointers the caller holds into the tree remain valid.
 */
void bst_erase(struct bst_root *root, struct bst_node *node)
{
	struct bst_node **link = bst_link(root, node);
	struct bst_node *child, *succ;

	/* Zero or one child: splice the child (maybe NULL) into our place. */
	if (!node->left || !node->right) {
		child = node->left ? node->left : node->right;
		if (child)
			child->parent = node->parent;
		*link = child;
		return;
	}

	/*
	 * Two children: the in-order successor is the leftmost node of the
	 * right subtree. It has no left child, so it can be lifted out and
	 * put in our place.
	 */
	succ = node->right;
	while (succ->left)
		succ = succ->left;

	if (succ != node->right) {
		/* Detach succ: its right subtree takes its old place. */
		succ->parent->left = succ->right;
		if (succ->right)
			succ->right->parent = succ->parent;

		succ->right = node->right;
		succ->right->parent = succ;
	}

	succ->left = node->left;
	succ->left->parent = succ;
	succ->parent = node->parent;
	*link = succ;
}

static struct bst_node *bst_leftmost(struct bst_node *node)
{
	if (node)
		while (node->left)
			node = node->left;
	return node;
}

static struct bst_node *bst_rightmost(struct bst_node *node)
{
	if (node)
		while (node->right)
			node = node->right;
	return node;
}

struct bst_node *bst_first(const struct bst_root *root)
{
	return bst_leftmost(root->node);
}

struct bst_node *bst_last(const struct bst_root *root)
{
	return bst_rightmost(root->node);
}

/*
 * In-order successor. Either the leftmost node of our right subtree, or
 * the first ancestor we reach while climbing up from a left child.
 */
struct bst_node *bst_next(const struct bst_node *node)
{
	struct bst_node *parent;

	if (node->right)
		return bst_leftmost(node->right);

	while ((parent = node->parent) && node == parent->right)
		node = parent;
	return parent;
}

/* Mirror image of bst_next(). */
struct bst_node *bst_prev(const struct bst_node *node)
{
	struct bst_node *parent;

	if (node->left)
		return bst_rightmost(node->left);

	while ((parent = node->parent) && node == parent->left)
		node = parent;
	return parent;
}
