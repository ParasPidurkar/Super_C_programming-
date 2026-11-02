# C11 threads (`<threads.h>`)

Since C11, threads are part of the **C language standard** itself, not just of POSIX. The header is `<threads.h>`. Any compiler and C library that support it give you the same API on Linux, Windows (MSVC 2022+), the BSDs and others.

On Linux, glibc 2.28+ implements `<threads.h>` as a thin layer on top of pthreads. A C11 thread **is** a pthread, so speed, scheduling and memory sharing are identical. Only the API is different (and smaller).

If you haven't used threads before, read [`../03_threads`](../03_threads/) first. It explains races, mutexes and condition variables step by step. This folder shows the same ideas with the standard API.

## Contents

1. [Build and run](#build-and-run)
2. [The API at a glance](#the-api-at-a-glance)
3. [pthreads vs `<threads.h>`](#pthreads-vs-threadsh)
4. [The example, part by part](#the-example-part-by-part)
5. [Which one should I use day to day?](#which-one-should-i-use-day-to-day)
6. [Pitfalls](#pitfalls)

---

## Build and run

```sh
make          # build
make run      # run the example
make check    # rebuild and run with AddressSanitizer + UBSan
make clean
```

By hand:

```sh
gcc -std=c11 -Wall -Wextra c11_threads.c -o c11_threads -pthread
./c11_threads
```

You still need `-pthread`, because glibc builds `<threads.h>` on pthreads.

There is no `make tsan`. ThreadSanitizer (gcc 13 and clang 18 on Ubuntu 24.04) cannot follow threads started by glibc's `thrd_create`, and crashes at startup even on a four-line program. Use `../03_threads` to try TSan.

---

## The API at a glance

| Group | Types and functions |
|-------|---------------------|
| threads | `thrd_t`, `thrd_create`, `thrd_join`, `thrd_detach`, `thrd_exit`, `thrd_current`, `thrd_equal`, `thrd_sleep`, `thrd_yield` |
| mutexes | `mtx_t`, `mtx_init`, `mtx_lock`, `mtx_trylock`, `mtx_timedlock`, `mtx_unlock`, `mtx_destroy` |
| condition variables | `cnd_t`, `cnd_init`, `cnd_wait`, `cnd_timedwait`, `cnd_signal`, `cnd_broadcast`, `cnd_destroy` |
| per-thread data | `thread_local`, `tss_t`, `tss_create`, `tss_get`, `tss_set`, `tss_delete` |
| run once | `once_flag`, `ONCE_FLAG_INIT`, `call_once` |

Every function returns a status:

| Value | Meaning |
|-------|---------|
| `thrd_success` | it worked |
| `thrd_busy` | `mtx_trylock`: the lock is held by someone else |
| `thrd_timedout` | `mtx_timedlock` / `cnd_timedwait`: the deadline passed |
| `thrd_nomem` | out of memory |
| `thrd_error` | any other failure |

Like pthreads, these functions **return** the status and don't set `errno`, so `perror()` is useless. The example's `check()` helper turns the status into a message.

Mutex types for `mtx_init(&m, type)`:

| Type | Meaning |
|------|---------|
| `mtx_plain` | ordinary lock |
| `mtx_recursive` | the thread that holds it may lock it again |
| `mtx_timed` | also supports `mtx_timedlock` |
| `mtx_timed \| mtx_recursive` | both |

---

## pthreads vs `<threads.h>`

| pthreads | C11 `<threads.h>` |
|----------|-------------------|
| `void *fn(void *arg)` | `int fn(void *arg)` (returns an **int**) |
| `pthread_create(&t, &attr, fn, arg)` | `thrd_create(&t, fn, arg)` (no attributes) |
| `pthread_join(t, &void_ptr)` | `thrd_join(t, &int_result)` |
| `pthread_detach` / `pthread_exit` / `pthread_self` | `thrd_detach` / `thrd_exit` / `thrd_current` |
| `pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;` | `mtx_t m;` + `mtx_init(&m, mtx_plain)` (**no static initialiser**) |
| `pthread_cond_t` | `cnd_t` |
| `pthread_key_t` + destructor | `tss_t` + destructor |
| `pthread_once` | `call_once` |
| `__thread` / `_Thread_local` | `thread_local` |
| `nanosleep` | `thrd_sleep` |
| `clock_gettime(CLOCK_REALTIME)` for deadlines | `timespec_get(&ts, TIME_UTC)` |
| returns 0 or an `E...` error number | returns `thrd_success` or `thrd_...` |
| **only in pthreads:** stack size, scheduling priority, CPU affinity, read-write locks, barriers, spinlocks, cancellation, per-thread signal masks, process-shared mutexes, condvars on `CLOCK_MONOTONIC` | **not available** |
| POSIX semaphores (`sem_t`) | no semaphore type: build one from `mtx_t` + `cnd_t` |

---

## The example, part by part

All of it is in [`c11_threads.c`](c11_threads.c). It uses only standard C11: no `<pthread.h>`, `<unistd.h>` or `_POSIX_C_SOURCE`.

### 1. Create and join, several arguments, an int result

A thread function takes one `void *`, so several arguments go in a struct, one struct **per thread**. The thread writes its big result into the struct and returns a small `int` status, which `thrd_join` hands back.

```c
struct job { int id; long from, to; long result; };

static int sum_range(void *p)
{
	struct job *j = p;
	...
	j->result = s;
	return 0;
}

thrd_create(&tid[i], sum_range, &jobs[i]);
...
int status;
thrd_join(tid[i], &status);
```

### 2. A mutex

Four threads each do 100 000 `counter++` under an `mtx_t`. The total always comes out exactly 400 000. Note `mtx_init()` at run time and `mtx_destroy()` at the end: C11 has no `PTHREAD_MUTEX_INITIALIZER`.

### 3. Condition variables: producer/consumer

A 4-slot ring buffer with two condition variables, `not_full` and `not_empty`. One producer pushes 1..20 followed by one `STOP` marker per consumer. Two consumers pop and add up the values. The waits are in `while` loops, never `if`, because of spurious wakeups.

### 4. `thread_local`, `tss_t` and `call_once`

- `thread_local int calls_in_this_thread;` gives each thread its own counter, and main's copy stays 0.
- `tss_t buf_key` gives each thread its own `malloc`'d buffer, and the **destructor** `free_buf` frees it automatically when the thread exits.
- `call_once(&key_once, make_key)` creates the key exactly once, even with three threads racing to do it.

### 5. Timed lock and a detached thread

- Main holds an `mtx_timed` mutex for 300 ms. Another thread calls `mtx_timedlock` with a 100 ms deadline and gets `thrd_timedout`.
- A thread is detached with `thrd_detach`, so it can't be joined. Main still waits for it properly, using a `done` flag, a mutex and `cnd_timedwait`, instead of sleeping and hoping.

Deadlines are **absolute times**, not durations. `deadline_in(ms)` reads the clock with `timespec_get()` and adds the delay.

### Sample output

Thread lines can come out in a different order on each run.

```
1. split sum(1..1000000) across 4 threads
  thread 0: sum(1..250000) = 31250125000
  thread 2: sum(500001..750000) = 156250125000
  thread 1: sum(250001..500000) = 93750125000
  thread 3: sum(750001..1000000) = 218750125000
  total = 500000500000 (expected 500000500000)

2. 4 threads x 100000 locked increments
  counter = 400000 (expected 400000)

3. producer/consumer: 20 items, queue of 4, 2 consumers
  consumer 0: took 10 items, sum 100
  consumer 1: took 10 items, sum 110
  total = 210 (expected 210)

4. thread_local, tss_t with destructor, call_once
  call_once: key created (printed once)
  thread 1: 2 call(s), uses "buffer of thread 1"
  destructor: freeing "buffer of thread 1"
  thread 2: 3 call(s), uses "buffer of thread 2"
  destructor: freeing "buffer of thread 2"
  thread 0: 1 call(s), uses "buffer of thread 0"
  destructor: freeing "buffer of thread 0"
  main: calls_in_this_thread = 0 (its own copy)

5. mtx_timedlock with a deadline, and a detached thread
  impatient thread: gave up after 100 ms (thrd_timedout)
  impatient thread returned 1
  detached thread: finished, telling main
  main: detached thread is done
```

---

## Which one should I use day to day?

**On Linux, for normal work, use pthreads.** It's what nearly all real Linux code, libraries, documentation and job interviews use.

| Situation | Use | Why |
|-----------|-----|-----|
| Linux / embedded Linux / system programming | **pthreads** | full feature set, every example, man page and tool (gdb, TSan, Valgrind/Helgrind) supports it |
| Need stack size, priorities, CPU affinity, rwlocks, barriers, semaphores, signal handling with threads | **pthreads** | `<threads.h>` simply doesn't have them |
| Code must also build on Windows with MSVC, with no extra library | **`<threads.h>`** | the only thread API that's standard C on both |
| Small portable library that only needs create/join/mutex/condvar | **`<threads.h>`** | no dependency beyond standard C |
| Learning threads | **both**: learn pthreads first, then notice `<threads.h>` is the same ideas with shorter names | |
| Writing C++ | **neither**: use `std::thread` / `std::jthread` (see [`../04_std_thread`](../04_std_thread/)) | |

Why `<threads.h>` isn't more popular:
- It came very late to the platforms that matter: glibc only in 2018 (2.28), MSVC only in 2022, and macOS still doesn't ship it.
- It lacks semaphores, rwlocks, barriers, thread attributes and signal masks, which real programs often need.
- Tool support lags behind. This folder can't run under ThreadSanitizer, for example.
- On POSIX systems pthreads already **was** the portable standard.

You also don't have to choose just one. On Linux a `thrd_t` is a `pthread_t`, but mixing the two APIs on the same thread or mutex isn't guaranteed by any standard, so keep each object in one world.

---

## Pitfalls

- **Forgetting `mtx_init` / `cnd_init`.** There is no static initialiser, and an uninitialised `mtx_t` is undefined behaviour.
- **Using `mtx_timedlock` on a mutex created without `mtx_timed`.** Undefined behaviour.
- **Passing a duration as a deadline.** `cnd_timedwait(&cv, &m, &(struct timespec){1, 0})` means "until 1 second after 1 January 1970", which has already passed, so it returns at once.
- **Deadlines follow the wall clock.** `TIME_UTC` can jump if the system time changes. pthreads lets a condvar use `CLOCK_MONOTONIC`, and C11 doesn't.
- **Returning a pointer through the int result.** Pointers don't fit in an `int` on 64-bit systems. Return results through the argument struct.
- All the pthreads pitfalls still apply: passing `&i` from a loop, `if` instead of `while` around a wait, returning from `main` while threads still run, lock-order deadlocks. See [`../03_threads/README.md`](../03_threads/README.md#common-pitfalls).
