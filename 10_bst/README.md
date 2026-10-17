# bst

An unbalanced binary search tree with `int` keys. Nodes are intrusive: you embed a `struct bst_node` in your own struct and get back to it with `bst_entry()`.

## Layout

```
10_bst/
├── bst.h      public API
├── bst.c      implementation
├── test.c     unit tests
├── demo.c     interactive playground
└── Makefile   test / demo / check (ASan+UBSan) / clean
```

## Requirements

- A C11 compiler (`gcc` or `clang`, used as `cc`)
- `make`
- For `make check`: a compiler with AddressSanitizer and UBSan support (gcc and clang on Linux both have it)

## Build and run

Run these from inside `10_bst/`:

```sh
cd 10_bst
```

### 1. Unit tests

```sh
make test
./test
```

Expected output:

```
bst: ok
```

### 2. Tests under the sanitizers

```sh
make check
```

This builds `test-san` with `-fsanitize=address,undefined` and runs it. It prints `bst: ok` if there are no memory errors or undefined behaviour.

### 3. Interactive demo

```sh
make demo
./demo              # start with an empty tree
./demo 50 30 70     # optional: keys to add before the prompt appears
```

### 4. Clean up

```sh
make clean          # removes test, test-san and demo
```

The compiler flags are strict (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`), so any warning fails the build. To use a different compiler, run e.g. `make CC=clang test`.

## Demo commands

```
add <k> [k...]   insert keys (duplicates are rejected)
del <k> [k...]   erase keys
find <k>         search, printing the path taken from the root
next <k>         in-order successor of k
prev <k>         in-order predecessor of k
min / max        smallest / largest key
list             all keys in order (bst_first + bst_next)
rlist            all keys in reverse (bst_last + bst_prev)
inorder          same as list: left, node, right
preorder         node, left, right
postorder        left, right, node
show             draw the tree (right subtree on top)
clear            erase everything
help             list the commands
quit             exit (Ctrl-D works too)
```

## Example session

```
$ ./demo 50 30 70
  added 50 at depth 0
  added 30 at depth 1
  added 70 at depth 1
Type 'help' for commands.
      /-- 70
  50
      \-- 30
  nodes=3 height=2
bst> add 5
  added 5 at depth 2
      /-- 70
  50
      \-- 30
          \-- 5
  nodes=4 height=3
bst> inorder
  5 30 50 70
bst> preorder
  50 30 5 70
bst> postorder
  5 30 70 50
bst> quit
```

The tree is drawn sideways: the right subtree is above its parent and the left subtree is below it.

## Traversals

| Order     | Visits                  | API                                                   |
|-----------|-------------------------|-------------------------------------------------------|
| Inorder   | left, node, right       | `bst_first()` + `bst_next()`                          |
| Reverse   | right, node, left       | `bst_last()` + `bst_prev()`                           |
| Preorder  | node, left, right       | `bst_first_preorder()` + `bst_next_preorder()`        |
| Postorder | left, right, node       | `bst_first_postorder()` + `bst_next_postorder()`      |

None of these recurse or use a stack. They walk the tree using each node's parent pointer, so even a degenerate, list-shaped tree cannot overflow the stack:

```c
struct bst_node *n;

for (n = bst_first_preorder(&root); n; n = bst_next_preorder(n))
	printf("%d ", bst_entry(n, struct item, node)->key);
```

`bst_next_postorder(n)` never looks at `n`'s children. You can fetch the next node and then free `n`, which makes postorder the natural way to destroy a whole tree.

## Running the queue code too

The queue in `../09_queue` builds the same way. Its `README.md` has the full details.

```sh
cd ../09_queue
make test && ./test     # -> "lqueue: ok", "aqueue: ok"
make check              # tests under ASan + UBSan
make demo && ./demo 4   # interactive playground (arg = aqueue size, default 4)
make clean
```
