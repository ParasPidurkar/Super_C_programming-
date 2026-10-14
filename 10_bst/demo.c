// SPDX-License-Identifier: GPL-2.0
/*
 * demo - interactive playground for the binary search tree.
 *
 * Usage: ./demo [key...]     (keys are added before the prompt appears)
 * Type 'help' at the prompt for commands.
 */
#define _POSIX_C_SOURCE 200809L	/* isatty() under -std=c11 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bst.h"

struct item {
	int key;
	struct bst_node node;
};

static struct bst_root root = BST_ROOT;
static size_t count;

static int key_of(const struct bst_node *n)
{
	return bst_entry(n, struct item, node)->key;
}

static int item_cmp(const struct bst_node *a, const struct bst_node *b)
{
	int x = key_of(a), y = key_of(b);

	return (x > y) - (x < y);
}

static int item_key_cmp(const void *key, const struct bst_node *n)
{
	int x = *(const int *)key, y = key_of(n);

	return (x > y) - (x < y);
}

static unsigned int depth_of(const struct bst_node *n)
{
	unsigned int d = 0;

	while ((n = n->parent))
		d++;
	return d;
}

/*
 * Print the tree on its side: root on the left, right subtree above,
 * left subtree below. Visiting nodes in reverse order with bst_prev()
 * yields exactly that top-to-bottom order, so no recursion is needed.
 */
static void show(void)
{
	const struct bst_node *n;
	unsigned int d, height = 0;

	if (bst_empty(&root)) {
		puts("  (empty)");
		return;
	}

	for (n = bst_last(&root); n; n = bst_prev(n)) {
		d = depth_of(n);
		if (d + 1 > height)
			height = d + 1;
		printf("  %*s", (int)(4 * d), "");
		if (n->parent)
			fputs(n == n->parent->right ? "/-- " : "\\-- ", stdout);
		printf("%d\n", key_of(n));
	}
	printf("  nodes=%zu height=%u\n", count, height);
}

static void help(void)
{
	puts("Commands:\n"
	     "  add <k> [k...]   insert keys (duplicates are rejected)\n"
	     "  del <k> [k...]   erase keys\n"
	     "  find <k>         search, printing the path taken from the root\n"
	     "  next <k>         in-order successor of k\n"
	     "  prev <k>         in-order predecessor of k\n"
	     "  min / max        smallest / largest key\n"
	     "  list             all keys in order (bst_first + bst_next)\n"
	     "  rlist            all keys in reverse (bst_last + bst_prev)\n"
	     "  show             draw the tree (right subtree on top)\n"
	     "  clear            erase everything\n"
	     "  help             this text\n"
	     "  quit             exit (Ctrl-D works too)");
}

/* Parse @tok as an int. Return: 0 on success, -EINVAL otherwise. */
static int parse_key(const char *tok, int *key)
{
	char *end;
	long val;

	errno = 0;
	val = strtol(tok, &end, 10);
	if (end == tok || *end || errno || val < INT_MIN || val > INT_MAX) {
		printf("  '%s' is not an integer\n", tok);
		return -EINVAL;
	}
	*key = (int)val;
	return 0;
}

/* Return: the next key in @args, or -EINVAL with a message if none/bad. */
static int one_key(char *args, int *key)
{
	char *tok = strtok(args, " \t");

	if (!tok) {
		puts("  this command needs a key");
		return -EINVAL;
	}
	return parse_key(tok, key);
}

static void do_add(char *args)
{
	struct item *it;
	char *tok;
	int key;

	tok = strtok(args, " \t");
	if (!tok)
		puts("  usage: add <k> [k...]");

	for (; tok; tok = strtok(NULL, " \t")) {
		if (parse_key(tok, &key))
			return;

		it = malloc(sizeof(*it));
		if (!it) {
			perror("malloc");
			return;
		}
		it->key = key;
		if (bst_add(&root, &it->node, item_cmp)) {
			printf("  %d already present\n", key);
			free(it);
			continue;
		}
		count++;
		printf("  added %d at depth %u\n", key, depth_of(&it->node));
	}
}

static void do_del(char *args)
{
	struct bst_node *n;
	char *tok;
	int key;

	tok = strtok(args, " \t");
	if (!tok)
		puts("  usage: del <k> [k...]");

	for (; tok; tok = strtok(NULL, " \t")) {
		if (parse_key(tok, &key))
			return;

		n = bst_find(&root, &key, item_key_cmp);
		if (!n) {
			printf("  %d not found\n", key);
			continue;
		}

		/* Say which of the three erase cases this is. */
		if (n->left && n->right)
			printf("  deleted %d (two children: successor %d takes its place)\n",
			       key, key_of(bst_next(n)));
		else if (n->left || n->right)
			printf("  deleted %d (one child: %d moves up)\n", key,
			       key_of(n->left ? n->left : n->right));
		else
			printf("  deleted %d (leaf)\n", key);

		bst_erase(&root, n);
		free(bst_entry(n, struct item, node));
		count--;
	}
}

/* Walk down by hand so the path can be printed; bst_find() does the same. */
static void do_find(char *args)
{
	const struct bst_node *n = root.node;
	int key;

	if (one_key(args, &key))
		return;

	printf("  root");
	while (n && key_of(n) != key) {
		printf(" -> %d", key_of(n));
		n = key < key_of(n) ? n->left : n->right;
	}
	if (n)
		printf(" -> %d  found at depth %u\n", key, depth_of(n));
	else
		printf(" -> NULL  %d not found\n", key);
}

static void do_step(char *args, int forward)
{
	struct bst_node *n, *m;
	int key;

	if (one_key(args, &key))
		return;

	n = bst_find(&root, &key, item_key_cmp);
	if (!n) {
		printf("  %d not found\n", key);
		return;
	}
	m = forward ? bst_next(n) : bst_prev(n);
	if (m)
		printf("  %s(%d) = %d\n", forward ? "next" : "prev", key, key_of(m));
	else
		printf("  %d is the %s key\n", key, forward ? "largest" : "smallest");
}

static void do_end(int last)
{
	struct bst_node *n = last ? bst_last(&root) : bst_first(&root);

	if (n)
		printf("  %s = %d\n", last ? "max" : "min", key_of(n));
	else
		puts("  (empty)");
}

static void do_list(int reverse)
{
	struct bst_node *n;

	printf(" ");
	n = reverse ? bst_last(&root) : bst_first(&root);
	for (; n; n = reverse ? bst_prev(n) : bst_next(n))
		printf(" %d", key_of(n));
	puts(bst_empty(&root) ? "  (empty)" : "");
}

static void clear(void)
{
	struct bst_node *n;

	while ((n = bst_first(&root))) {
		bst_erase(&root, n);
		free(bst_entry(n, struct item, node));
	}
	count = 0;
}

int main(int argc, char **argv)
{
	char line[256], *cmd, *args;
	int i;

	for (i = 1; i < argc; i++)
		do_add(argv[i]);
	puts("Type 'help' for commands.");
	show();

	for (;;) {
		printf("bst> ");
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

		if (!strcmp(cmd, "add")) {
			do_add(args);
		} else if (!strcmp(cmd, "del")) {
			do_del(args);
		} else if (!strcmp(cmd, "clear")) {
			clear();
		} else if (!strcmp(cmd, "show")) {
			/* shown below */
		} else if (!strcmp(cmd, "find")) {
			do_find(args);
			continue;
		} else if (!strcmp(cmd, "next") || !strcmp(cmd, "prev")) {
			do_step(args, cmd[0] == 'n');
			continue;
		} else if (!strcmp(cmd, "min") || !strcmp(cmd, "max")) {
			do_end(cmd[1] == 'a');
			continue;
		} else if (!strcmp(cmd, "list") || !strcmp(cmd, "rlist")) {
			do_list(cmd[0] == 'r');
			continue;
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

	clear();
	return EXIT_SUCCESS;
}
