# queue

Two small FIFO queues in C, written in Linux-kernel style:

| Queue    | Files                  | Storage                         | Allocates?               | Capacity |
|----------|------------------------|---------------------------------|--------------------------|----------|
| `aqueue` | `aqueue.h`, `aqueue.c` | Array ring buffer, copies elements | Once, in `aqueue_init()` | Fixed, rounded up to a power of two |
| `lqueue` | `lqueue.h` (header only) | Intrusive singly linked list  | Never                    | Unbounded |

Both follow FIFO order: `push` adds at the tail, and `pop`/`peek` take from the head.

---

## Layout

```
queue/
├── aqueue.h   ring buffer API + inline helpers (size/len/empty/full)
├── aqueue.c   ring buffer implementation (init/free/push/pop/peek)
├── lqueue.h   linked-list queue, all static inline
├── test.c     assert-based unit tests for both queues
├── demo.c     interactive REPL to watch both queues work
└── Makefile   test / demo / check (ASan+UBSan) / clean
```

---

## Build and run

```sh
make test && ./test     # unit tests        -> "lqueue: ok", "aqueue: ok"
make check              # same tests under AddressSanitizer + UBSan
make demo && ./demo 4   # interactive playground (arg = aqueue size, default 4)
make clean
```

The compiler flags are strict (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`), so any warning fails the build.

### Demo commands

```
push <x> [x...]  aqueue: push integers      lqueue: push jobs named x
pop              remove the oldest item
peek             show the oldest item without removing it
show             print the queue's internal state
switch           toggle between aqueue and lqueue
wrap             (aqueue, empty only) set in/out to UINT_MAX - 1
help / quit      (Ctrl-D also quits)
```

The demo starts on `aqueue`. Type `switch` to use `lqueue`.

---

## aqueue: array ring buffer

### Data structure

```c
struct aqueue {
	unsigned int in;    /* total pushes ever (free-running) */
	unsigned int out;   /* total pops ever   (free-running) */
	unsigned int mask;  /* capacity - 1, capacity is 2^k     */
	size_t esize;       /* bytes per element                 */
	void *data;         /* capacity * esize bytes            */
};
```

The design has three key ideas:

1. **The capacity is always a power of two.** That lets a slot index be computed with `idx & mask` instead of `idx % size`, which is cheaper and well defined after the counter wraps.
2. **`in` and `out` are never reset.** They only count up, and they are allowed to overflow past `UINT_MAX` back to 0.
3. **The element count is always `in - out`.** Unsigned subtraction is modulo 2^32, so this stays correct even after `in` has wrapped and `out` has not. All slots can be used, so there is no wasted "sentinel" slot and no separate `full` flag.

The helpers in `aqueue.h` all follow from these:

```c
size  = mask + 1
len   = in - out
empty = in == out
full  = len > mask        /* i.e. len == size */
```

### Workflow

**`aqueue_init(q, size, esize)`**

1. Rejects `size == 0`, `esize == 0`, or `size > 2^31` (rounding that up would overflow) and returns `-EINVAL`.
2. Rounds `size` up to the next power of two with the bit-smearing trick in `roundup_pow_of_two()`. It does `n--`, then ORs `n` with itself shifted right by 1, 2, 4, 8 and 16 to set every bit below the highest one, then does `+1`. For example, 5 becomes 8.
3. Allocates the buffer with `calloc(size, esize)`. Returns `-ENOMEM` if that fails.
4. Sets `in = out = 0` and `mask = size - 1`.

**`aqueue_push(q, elem)`**

```
if full -> -ENOSPC
memcpy(slot[in & mask], elem, esize)
in++
```

**`aqueue_peek(q, elem)`**

```
if empty -> -ENODATA
memcpy(elem, slot[out & mask], esize)        /* out unchanged */
```

**`aqueue_pop(q, elem)`**

```
if empty -> -ENODATA
if elem != NULL: memcpy(elem, slot[out & mask], esize)
out++
```

Passing `NULL` discards the element.

**`aqueue_free(q)`** frees `data` and zeroes the fields.

`aqueue_slot()` turns a counter into a byte address: `data + (idx & mask) * esize`. The queue copies elements by value, so it can hold any fixed-size type.

### Walkthrough (capacity 4)

```
start          in=0 out=0 len=0  [ .  .  .  . ]
push 1 2 3 4   in=4 out=0 len=4  [ 1  2  3  4 ]   full
push 5         -> -ENOSPC
pop  -> 1      in=4 out=1 len=3  [ .  2  3  4 ]
push 7         in=5 out=1 len=4  [ 7  2  3  4 ]   slot = 4 & 3 = 0, wrapped around the array
```

### Counter wraparound

The `wrap` demo command (and the test in `test.c`) sets `in = out = UINT_MAX - 1`:

```
wrap           in=4294967294 out=4294967294 len=0   head->slot 2
push 1 2 3     in=1          out=4294967294 len=3   [ 3 . 1 2 ]
```

After the third push, `in` has overflowed to 1. But `1 - 4294967294` in 32-bit unsigned arithmetic is 3, so the count is still right, and `& mask` still picks the right slots. No special case is needed.

---

## lqueue: intrusive linked list

### Data structure

```c
struct lqueue_node { struct lqueue_node *next; };

struct lqueue {
	struct lqueue_node  *head;   /* oldest node, NULL if empty           */
	struct lqueue_node **tail;   /* &last->next, or &head when empty     */
	size_t len;
};
```

**Intrusive** means the queue never allocates or owns memory. You put a `struct lqueue_node` inside your own struct, and the queue links those nodes together:

```c
struct job {
	int id;
	char name[32];
	struct lqueue_node node;   /* the queue's hook */
};
```

To get from a node back to the struct around it, use `lqueue_entry(ptr, type, member)`. It is `container_of`: it subtracts `offsetof(type, member)` from the node's address.

```c
struct job *j = lqueue_entry(n, struct job, node);
```

This is the same pattern as the kernel's `list_head`.

### The pointer-to-pointer `tail`

`tail` does not point at the last node. It points at the **`next` field** of the last node, or at `head` itself when the queue is empty. So push is always "write to `*tail`", with no "is the list empty?" branch:

```
empty:       head = NULL          tail = &head
                ^-------------------'

after A:     head -> [A|next=NULL]
                        ^---------- tail = &A.next

after B:     head -> [A|next] -> [B|next=NULL]
                                   ^---------- tail = &B.next
```

### Workflow

**Init:** use `LQUEUE_INIT(name)` for a static initializer, or call `lqueue_init(&q)` at runtime. Both set `head = NULL`, `tail = &head` and `len = 0`.

**`lqueue_push(q, n)`** runs in O(1):

```
n->next = NULL
*q->tail = n          /* link after the last node, or set head */
q->tail  = &n->next
q->len++
```

**`lqueue_peek(q)`** returns `head`, or `NULL` if the queue is empty.

**`lqueue_pop(q)`** runs in O(1):

```
n = head; if !n -> NULL
head = n->next
if head == NULL: tail = &head    /* queue became empty: reset tail */
len--
return n
```

Resetting `tail` matters. Without it, `tail` would still point at the popped node's `next`, and the next push would be written into a node that is no longer in the queue. That push would be lost. `test.c` checks this case directly.

### Rules for callers

- **Don't copy a `struct lqueue` by value.** When the queue is empty, `tail` points into the struct itself (`&q->head`), so a copy would point back at the original.
- **A node can be in only one lqueue at a time.** It has a single `next` field.
- **You own the memory.** The queue never frees anything. In `demo.c`, `push` mallocs each `struct job`, and `pop` frees the job after taking it out. On exit, the demo drains the queue and frees every remaining job.

---

## aqueue vs lqueue

|                     | aqueue                         | lqueue                              |
|---------------------|--------------------------------|-------------------------------------|
| Memory              | One allocation up front        | None; the caller owns the nodes     |
| Element             | Copied in and out (`memcpy`)   | Linked in place, no copy            |
| Capacity            | Fixed (2^k); full returns `-ENOSPC` | Unlimited                      |
| Empty pop/peek      | `-ENODATA`                     | `NULL`                              |
| Cache behaviour     | Contiguous, cache friendly     | Pointer chasing                     |
| Best for            | Small fixed-size values, bounded buffers | Objects that already exist, unbounded queues |

Neither queue is thread-safe. If several threads share one, the caller must add locking.

---

## Tests (`test.c`)

**`test_lqueue`**
- A new queue is empty, and popping it returns `NULL`.
- Pushing 5 jobs gives `len == 5`, and peek returns job 0.
- Popping returns jobs 0..4 in order, and the queue ends up empty.
- Pushing after the queue was drained still works, which proves `tail` was reset.

**`test_aqueue`**
- `init` with size 0 or esize 0 returns `-EINVAL`.
- `init(5)` gives size 8, and pop/peek on the empty queue return `-ENODATA`.
- Filling all 8 slots works, and a 9th push returns `-ENOSPC`.
- Peek returns 0, and pops come back in order 0..7.
- With the counters set to `UINT_MAX - 2`, pushing and popping 8 items still gives the right length and order across the overflow.

Run `make check` to repeat the tests under the sanitizers. That catches out-of-bounds access, use-after-free and undefined behaviour such as signed overflow.

---

## Demo internals (`demo.c`)

```
main
 ├─ parse argv[1] as the aqueue size, then aqueue_init(&aq, size, sizeof(int))
 ├─ loop: print prompt -> fgets -> strtok the command
 │    push   -> do_push : aqueue_push(int) | malloc job + lqueue_push
 │    pop    -> do_pop(1): aqueue_pop      | lqueue_pop + free
 │    peek   -> do_pop(0): aqueue_peek     | lqueue_peek
 │    switch -> toggle use_lqueue
 │    wrap   -> in = out = UINT_MAX - 1 (aqueue must be empty)
 │    then show(): aqueue_show prints in/out/len plus the slot array
 │                 lqueue_show walks head -> ... -> NULL and the tail state
 └─ on quit/EOF: drain lqueue (free each job), aqueue_free
```

`aqueue_show` decides whether slot `i` is in use with `((i - out) & mask) < len`. In words: slot `i` is in use if it is fewer than `len` steps after the head slot, counting around the ring.
