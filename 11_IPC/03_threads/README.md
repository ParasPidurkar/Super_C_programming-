# Threads (POSIX pthreads)

A thread is a second flow of execution **inside one process**. Processes talk through the kernel (pipes, message queues, signals). Threads skip that: they share the same memory, so they can talk through plain variables. That is what makes them fast, and it is also what makes them dangerous.

```
            one process
┌──────────────────────────────────────────────┐
│  code · globals · heap · open files          │  ← shared by every thread
│                                              │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐  │
│  │ thread 1 │   │ thread 2 │   │ thread 3 │  │
│  │ stack    │   │ stack    │   │ stack    │  │  ← private to each thread
│  │ registers│   │ registers│   │ registers│  │
│  │ sig mask │   │ sig mask │   │ sig mask │  │
│  └──────────┘   └──────────┘   └──────────┘  │
└──────────────────────────────────────────────┘
```

A simple way to picture it: a **process** is a kitchen and a **thread** is a cook. All the cooks in one kitchen share the same fridge and table (memory). They can work at the same time, which is fast. But if two cooks grab the same bowl at the same moment, the dish gets ruined. Most of this folder is about making the cooks take turns safely.

## Contents

1. [Words you need first](#words-you-need-first)
2. [Build and run](#build-and-run)
3. [Threads vs processes](#threads-vs-processes)
4. [The examples, one by one](#the-examples-one-by-one)
5. [Which tool should I use?](#which-tool-should-i-use)
6. [API reference](#api-reference)
7. [Alternatives to pthreads](#alternatives-to-pthreads)
8. [Inspecting threads from the shell](#inspecting-threads-from-the-shell)
9. [Common pitfalls](#common-pitfalls)
10. [Exercises](#exercises)

---

## Words you need first

| Word | Meaning in plain English |
|------|--------------------------|
| **Thread** | One flow of execution. Every program starts with one, the *main thread*, which runs `main()`. |
| **Concurrent / parallel** | Several threads make progress at the same time. On a multi-core CPU they really run at the same instant. |
| **Shared data** | Globals, `static` variables and anything on the heap (`malloc`). Every thread can see and change it. |
| **Private data** | Local variables. Each thread has its own stack, so its locals are its own. |
| **Join** | Wait for a thread to finish and collect its result, like `waitpid()` for a child process. |
| **Detach** | Say "I will never join this thread". It cleans up after itself when it ends. |
| **Data race** | Two threads touch the same variable at the same time, at least one writes, and nothing makes them take turns. The result is unpredictable. |
| **Critical section** | A piece of code that only one thread may run at a time. |
| **Mutex** | A lock. `lock()` enters the critical section, `unlock()` leaves it. A thread that tries to lock a locked mutex sleeps until it is free. |
| **Atomic** | An operation the CPU does in one indivisible step, so no other thread can see it half done. |
| **Condition variable** | A place where threads sleep until another thread says "the data changed, check again". |
| **Semaphore** | A counter of "tokens". Take one to go in, give it back when you leave. If none are left, you wait. |
| **Deadlock** | Two threads each wait for a lock the other one holds. Both wait forever. |
| **Thread-safe** | A function that works correctly when several threads call it at once. |

---

## Build and run

You need Linux, a C11 compiler and `make`.

```sh
cd 11_IPC/03_threads

make            # build everything
make run        # run examples 01-06 one after another
make check      # run them under AddressSanitizer + UBSan
make tsan       # run the race-free ones under ThreadSanitizer
make clean      # delete the binaries
```

To build and run **just one** example:

```sh
make 02_race_mutex && ./02_race_mutex

# or by hand, without make:
gcc -std=c11 -Wall -pthread 02_race_mutex.c -o 02_race_mutex
./02_race_mutex
```

The compiler flags are strict (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`), so any warning fails the build. Always compile and link with `-pthread`: it links libpthread and sets the right macros. If you forget it you get errors like `undefined reference to pthread_create` (on older glibc).

What `make check` and `make tsan` do:

- **AddressSanitizer (ASan)** catches memory bugs: use-after-free, buffer overflows, leaks.
- **UndefinedBehaviorSanitizer (UBSan)** catches things like signed integer overflow.
- **ThreadSanitizer (TSan)** catches data races. 02 is left out because it races on purpose.

| File                    | Teaches                                                                 |
|-------------------------|-------------------------------------------------------------------------|
| `common.h`              | `die()`, `check()` (for pthread error codes), `msleep()`, `now()`        |
| `01_create_join.c`      | `pthread_create`, passing arguments, `pthread_join`, return values, detached threads |
| `02_race_mutex.c`       | a real data race, then fixed with a mutex and with C11 atomics          |
| `03_condvar.c`          | condition variables: producer/consumer over a bounded buffer            |
| `04_semaphore.c`        | POSIX semaphores: limit concurrency, and signal "work finished"         |
| `05_tls_once.c`         | `_Thread_local`, `pthread_key_create` with a destructor, `pthread_once` |
| `06_thread_signals.c`   | signals + threads: block everywhere, handle in one `sigwait()` thread   |

Work through them in order: each one builds on the one before. Every source file is heavily commented, so read the code alongside this README.

The same examples written with the C++ standard thread library are in [`../04_std_thread`](../04_std_thread/).

### The helpers in `common.h`

| Helper | What it does | Why it exists |
|--------|--------------|---------------|
| `die(what)` | `perror(what)` then `exit(1)` | For normal calls (`malloc`, `sem_init`) that set `errno` on failure. |
| `check(err, what)` | if `err != 0`, print `strerror(err)` and exit | `pthread_*` functions **return** the error code instead of setting `errno`, so `perror` would print the wrong message. |
| `msleep(ms)` | sleep for `ms` milliseconds | Stands in for "doing work" so threads visibly overlap. Restarts if a signal interrupts it. |
| `now()` | current time in seconds (monotonic clock) | To time things. Only the difference between two calls means anything. |

Each `.c` file starts with `#define _POSIX_C_SOURCE 200809L`. With `-std=c11`, the compiler hides everything that is not plain ISO C. That define turns the POSIX functions (pthreads, `nanosleep`, `clock_gettime`) back on.

---

## Threads vs processes

|                        | `fork()` process                     | `pthread_create()` thread             |
|------------------------|--------------------------------------|---------------------------------------|
| Memory                 | separate copy (copy-on-write)        | **shared**                            |
| Communicating          | needs IPC: pipes, queues, shm…       | ordinary variables + locking          |
| Creation cost          | higher                               | lower                                 |
| A crash (`SIGSEGV`)    | kills only that process              | kills **the whole process**           |
| Wait for it to finish  | `waitpid()`                          | `pthread_join()`                      |
| Bugs to watch for      | lost IPC messages                    | data races, deadlocks                 |

On Linux both are "tasks" created by `clone()`; a thread is just a task that shares its parent's memory, files and signal handlers.

---

## The examples, one by one

> **Your output will differ from what is shown below.** Threads run in an order the OS chooses, so line order, item counts and timings change from run to run. That is normal, and seeing it is part of the lesson. The *final* totals are always the same.

### 01 — creating and joining threads

**Goal:** start threads, give each one its input, and collect the results.

```c
int pthread_create(pthread_t *tid, const pthread_attr_t *attr,
                   void *(*fn)(void *), void *arg);
int pthread_join(pthread_t tid, void **retval);
```

| Argument | Meaning |
|----------|---------|
| `tid` | where the new thread's ID is written |
| `attr` | settings (detached, stack size…); `NULL` = defaults |
| `fn` | the function the thread starts in. It **must** have the form `void *fn(void *)` |
| `arg` | the single `void *` handed to `fn` |

**What the program does, in three parts:**

1. **Split a big job.** Sum the numbers 1 to 1,000,000 with 4 threads, each doing a quarter:

   ```
   main ──┬─ create T0: sum(1..250000)        ─┐
          ├─ create T1: sum(250001..500000)   ─┤  all four run
          ├─ create T2: sum(500001..750000)   ─┤  at the same time
          ├─ create T3: sum(750001..1000000)  ─┘
          │
          └─ join T0, T1, T2, T3  →  add the four results
   ```

   A thread function takes only one argument, so the inputs (`id`, `from`, `to`) go into a `struct job`, and the thread writes its answer into the same struct (`result`). Each thread gets its **own** struct, `&jobs[i]`, so no locking is needed.

2. **Return a value.** A thread computes `7 * 7` and returns a pointer to the answer. `pthread_join(t, &ret)` hands that pointer to `main`. The result is `malloc`'d because the thread's stack is gone once it exits; `main` frees it.

3. **Detached thread.** A thread created with `PTHREAD_CREATE_DETACHED` frees itself when it ends. You can't join it, so `main` sleeps briefly to let it finish before exiting.

**What you'll see:**

```
1. split sum(1..1000000) across 4 threads
  thread 1: sum(250001..500000) = 93750125000
  thread 0: sum(1..250000) = 31250125000
  thread 2: sum(500001..750000) = 156250125000
  thread 3: sum(750001..1000000) = 218750125000
  total = 500000500000 (expected 500000500000)
2. get a heap-allocated return value through pthread_join()
  square(7) = 49
3. a detached thread cleans itself up; it can't be joined
  detached thread: finished, nobody will join me
```

Here thread 1 printed before thread 0, even though it was created later. Threads finish in any order. The total is still right because `main` only adds results **after** joining.

**Remember:**

- Pass each thread **its own** argument. Passing `&i` from the loop is a classic bug: `i` changes before the thread reads it, so several threads see the same number.
- A returned pointer must not point at the thread's stack, which is gone after it exits. Use the heap.
- Every thread must be either **joined** or **detached**, or it leaks like a zombie process.
- Returning from `main()` calls `exit()`, which kills all threads at once. Join them first, or end `main` with `pthread_exit(NULL)`.

### 02 — data races, mutexes and atomics

**Goal:** see a data race happen, then fix it two ways.

Four threads each add 1 to a shared counter one million times, so the answer should be 4,000,000. The program does this three times, with three different counters:

| Counter | Protection | Function |
|---------|------------|----------|
| `racy` | none (**wrong** on purpose) | `inc_racy` |
| `locked` | a `pthread_mutex_t` around `locked++` | `inc_locked` |
| `atomic_counter` | `atomic_fetch_add()` | `inc_atomic` |

**Why the racy one is wrong.** `counter++` looks like one step but the CPU does three: *load* the value, *add* 1, *store* it back. Two threads can mix those steps:

```
 thread A            thread B            counter
 load  (5)                                  5
                     load  (5)              5
 add   -> 6                                 5
                     add   -> 6             5
 store 6                                    6
                     store 6                6   <- should be 7!
```

One increment is lost. With 4 threads hammering the same variable, millions get lost.

**What you'll see:**

```
4 threads each increment a shared counter 1000000 times
  racy     1041067 / 4000000  lost 2958933     6.7 ms
  mutex    4000000 / 4000000  lost       0   250.7 ms
  atomic   4000000 / 4000000  lost       0    36.4 ms
racy loses updates (the exact number changes every run);
mutex and atomic are always exact; atomic is cheaper.
```

How to read it:

- **racy** is fast but wrong: about 3 million of the 4 million increments vanished. Run it again and you'll get a different wrong number.
- **mutex** is always correct but the slowest, because every one of the 4 million increments locks and unlocks.
- **atomic** is always correct and much faster than the mutex, because the CPU does load+add+store as one instruction (`lock xadd` on x86).

| Fix               | How                                                      | Cost   |
|-------------------|----------------------------------------------------------|--------|
| `pthread_mutex_t` | `lock(); counter++; unlock();` makes it one critical section | highest |
| `atomic_long`     | `atomic_fetch_add(&c, 1)` uses a single locked CPU instruction | lower  |
| No sharing        | each thread counts locally, sum after join (see 01)      | none   |

Use atomics for single counters and flags. Use a mutex as soon as **several** variables must change together. Best of all, avoid sharing in the first place.

`racy` is declared `volatile` only so the optimiser doesn't turn the loop into a single `+= 1000000`, which would hide the bug. `volatile` does **not** make code thread-safe.

`make tsan` leaves out 02 because ThreadSanitizer correctly reports its race. Try it yourself:

```sh
cc -std=c11 -g -pthread -fsanitize=thread 02_race_mutex.c -o race
setarch $(uname -m) -R ./race      # "WARNING: ThreadSanitizer: data race"
```

### 03 — condition variables

**Goal:** let threads *wait* for data efficiently, using the producer/consumer pattern.

**The setup:** 2 producers each make 10 numbers (producer 1 makes 101..110, producer 2 makes 201..210). They put them into a shared buffer with only **4 slots**. 3 consumers take numbers out and add them up.

```
 producer 1 ─┐                      ┌─► consumer A
             ├─► [ buffer: 4 slots ]├─► consumer B
 producer 2 ─┘                      └─► consumer C
```

Two kinds of waiting are needed:

- A consumer that finds the buffer **empty** must wait for a producer.
- A producer that finds the buffer **full** must wait for a consumer.

A busy loop (`while (count == 0) {}`) would burn 100% CPU. A **condition variable** lets the thread sleep until it's told something changed.

**The buffer** is a ring (circular queue): `head` is where the next item is taken from, `tail` is where the next one goes, `count` is how many are stored. The indexes wrap round with `% CAP`. One mutex, `lock`, protects all of it.

**The two condition variables:**

| Name | Who sleeps on it | Who wakes them |
|------|------------------|----------------|
| `not_empty` | consumers, when `count == 0` | `put()`, after adding an item |
| `not_full` | producers, when `count == CAP` | `get()`, after removing an item |

**The waiting pattern** (this is the heart of the example):

```c
pthread_mutex_lock(&lock);
while (count == 0)                      /* ALWAYS a while, never an if */
        pthread_cond_wait(&not_empty, &lock);   /* unlock + sleep + relock */
/* ... take an item ... */
pthread_cond_signal(&not_full);
pthread_mutex_unlock(&lock);
```

`pthread_cond_wait` does three things: it **unlocks** the mutex so others can change the data, **sleeps** until signalled, then **locks** the mutex again before returning. Use `while`, not `if`: wakeups can be spurious, and another consumer may take the item before you get the lock back. So always re-check.

**How it stops.** Consumers sleep inside `get()`, so they can't check a "stop" flag. When both producers are done, `main` puts one `STOP` value (`-1`) into the buffer per consumer: a "poison pill". Each consumer takes one and exits. The buffer is first-in-first-out, so every real item is taken before any pill.

**What you'll see:**

```
2 producers x 10 items -> buffer[4] -> 3 consumers
  consumer got  8 items, sum 1241
  consumer got  5 items, sum 729
  consumer got  7 items, sum 1140
  total 3110, expected 3110: every item delivered exactly once
```

How the work splits between consumers (8/5/7) changes on every run. The total is always 3110 (101+…+110 + 201+…+210), which proves no item was lost or delivered twice.

### 04 — semaphores

**Goal:** allow at most N threads into a section at once, and wait for detached threads to finish.

A semaphore is a counter that never goes below zero:

- `sem_wait(&s)`: "take a token". If the count is above 0, decrement it and continue. If it is 0, sleep until someone posts.
- `sem_post(&s)`: "give a token back". Increment it, waking one sleeper.

Think of a car park with 2 spaces. A car (thread) can only enter if a space is free. When a car leaves, the next waiting car gets in.

**The setup:** 6 workers, but only 2 "slots". The program uses two semaphores:

| Semaphore | Starts at | Used for |
|-----------|-----------|----------|
| `slots` | 2 | at most 2 workers in the limited section at once |
| `done` | 0 | each worker posts when it finishes; `main` waits 6 times |

```
 worker:   sem_wait(&slots)   ← take a slot (sleep if none free)
           ... work 30 ms ...
           sem_post(&slots)   ← give it back (wakes a waiting worker)
           sem_post(&done)    ← tell main "I'm finished"

 main:     6 × sem_wait(&done)
```

The workers are **detached**, so `main` can't join them. The `done` semaphore is how `main` knows they've all finished. Unlike `pthread_cond_signal`, a post is **remembered**: if a worker posts before `main` starts waiting, the count stays raised and `main`'s `sem_wait` returns at once.

To prove the limit works, the workers count how many are inside with atomics, and record the highest number seen (`max_inside`).

**What you'll see:**

```
6 workers, 2 slots
  worker 0 in  (1 inside)
  worker 1 in  (2 inside)
  worker 1 out
  worker 0 out
  worker 2 in  (1 inside)
  worker 4 in  (2 inside)
  worker 2 out
  worker 4 out
  worker 3 in  (1 inside)
  worker 5 in  (2 inside)
  worker 3 out
  worker 5 out
  never more than 2 inside at once (limit 2)
```

Workers go in two at a time. A new one only enters after one leaves. Which worker gets the free slot (here 4 before 3) is up to the scheduler.

**Remember:**

- Initialised to **N**: "at most N at a time" (connection pools, rate limits).
- Initialised to **0**: "something happened" signal between threads.
- Unlike a mutex, any thread may `sem_post`, not only the one that waited.
- `sem_*` functions set `errno` (use `die()`), unlike `pthread_*` (use `check()`).
- `sem_init(&s, 1, n)` with the `sem_t` in shared memory works across processes too. Named semaphores (`sem_open`) are the IPC version.

### 05 — thread-local storage and `pthread_once`

**Goal:** give each thread its own copy of a "global" variable, and run setup code exactly once.

**Why you'd want this.** A normal global has one copy shared by all threads. Sometimes each thread needs its own. The famous example is `errno`: if it were shared, thread A's failed call could overwrite the error thread B is about to read. So `errno` is thread-local.

The program shows two ways to get per-thread data, plus one-time initialisation:

| Tool | What it gives you | Used in the code for |
|------|-------------------|----------------------|
| `_Thread_local int my_count;` | each thread has its own `my_count`, starting at 0 | a simple per-thread counter |
| `pthread_key_create(&key, destructor)` | a slot holding one `void *` per thread. The destructor runs automatically when the thread exits | a per-thread `malloc`'d buffer that frees itself |
| `pthread_once(&once, init)` | runs `init()` exactly once, even if many threads call it at the same moment | creating the key the first time it's needed |

**What happens:** 3 threads run, one after another so the output is easy to read. Thread 0 adds 1 to `my_count`, thread 1 adds 2, thread 2 adds 3. Each also gets its own buffer through `thread_buf()` and writes its name into it.

**What you'll see:**

```
  init: runs exactly once
  thread 0: my_count = 1, buf = "buffer of thread 0"
  destructor: freeing "buffer of thread 0"
  thread 1: my_count = 2, buf = "buffer of thread 1"
  destructor: freeing "buffer of thread 1"
  thread 2: my_count = 3, buf = "buffer of thread 2"
  destructor: freeing "buffer of thread 2"
  main: my_count = 0 (its own copy, untouched)
```

How to read it:

- `init` appears **once**, although all 3 threads called `pthread_once`.
- `my_count` is 1, 2, 3, not 1, 3, 6: each thread started from **its own** 0.
- After each thread ends, the destructor frees **that** thread's buffer. The code never calls `free(b)` itself.
- `main` is a thread too. Its own `my_count` was never touched, so it is still 0.

### 06 — signals in a threaded program

**Goal:** handle signals like Ctrl-C or `kill -HUP` safely when several threads are running.

(Read [`../02_signals`](../02_signals/) first if signals are new to you.)

**The problem.** Signal handlers are shared by the whole process, but a signal sent to the process is delivered to **any one** thread that hasn't blocked it, and the kernel picks which. That thread could be holding a mutex the handler then needs: a deadlock. Handlers also may not call `printf`, `malloc` or `pthread_mutex_lock`.

**The clean pattern, in three steps:**

1. In `main`, **before creating any thread**, block the signals with `pthread_sigmask(SIG_BLOCK, …)`. New threads inherit that mask, so no thread gets interrupted.
2. Create **one** signal thread that loops on `sigwait()`. Blocked signals stay *pending*. `sigwait()` sleeps until one is pending, takes it, and returns its number. That thread is ordinary code, not a handler, so it may lock mutexes and call `printf`.
3. That thread tells the others what to do, here through an `atomic_bool stop` flag.

```
 kill(pid, SIGHUP) ──► process ──► blocked in every thread ──► pending
                                                                 │
                         signal thread: sigwait() ◄──────────────┘
                           SIGHUP           → "reload config", keep going
                           SIGTERM / SIGINT → stop = 1, exit
                                                │
 worker 0, worker 1:  while (!stop) work; ◄─────┘  then finish cleanly
```

**What happens:** `main` starts the signal thread and 2 workers, then plays "the outside world": it sends itself `SIGHUP` twice and then `SIGTERM`, exactly like `kill -HUP <pid>` and `kill <pid>` from a shell would.

**What you'll see:**

```
main: kill(getpid(), SIGHUP) twice, then SIGTERM
  signal thread: SIGHUP -> reload config
  signal thread: SIGHUP -> reload config
  signal thread: SIGTERM -> asking workers to stop
  worker 1: stopped cleanly after 8 ticks
  worker 0: stopped cleanly after 8 ticks
main: all threads joined, 2 reload(s) handled
```

Every signal was handled by the signal thread, never by a worker. On `SIGTERM` the workers weren't killed mid-work: they saw the flag, finished their loop and printed a clean goodbye.

Use `pthread_sigmask`, not `sigprocmask`: what `sigprocmask` does in a multithreaded program is unspecified. `signalfd()` (in `../02_signals`) is the `poll()`-based variant of the same idea.

---

## Which tool should I use?

| I want to… | Use | Example |
|------------|-----|---------|
| run work in parallel and collect the results | `pthread_create` + `pthread_join`, one argument struct per thread | 01 |
| update one shared counter or flag | `atomic_*` | 02, 06 |
| update several shared variables together | `pthread_mutex_t` | 02, 03 |
| sleep until some data is ready | condition variable (+ its mutex) | 03 |
| allow at most N threads at once | semaphore initialised to N | 04 |
| wait for detached threads to finish | semaphore initialised to 0 | 04 |
| give each thread its own copy of a global | `_Thread_local` or `pthread_key_create` | 05 |
| run setup code exactly once | `pthread_once` | 05 |
| handle signals safely with threads | block in `main` + one `sigwait()` thread | 06 |
| many readers, occasional writer | `pthread_rwlock_t` | (exercise) |
| make N threads meet at one point | `pthread_barrier_t` | (exercise 4) |

---

## API reference

| Call | Purpose |
|------|---------|
| `pthread_create(&t, attr, fn, arg)` | start a thread running `fn(arg)` |
| `pthread_join(t, &ret)` | wait for `t`, get its return value |
| `pthread_detach(t)` | let `t` free itself on exit |
| `pthread_exit(ret)` | end the calling thread |
| `pthread_self()` / `pthread_equal(a, b)` | own ID / compare IDs |
| `pthread_mutex_lock/trylock/unlock` | mutual exclusion |
| `pthread_cond_wait/timedwait/signal/broadcast` | sleep until a condition / wake one / wake all |
| `pthread_rwlock_rdlock/wrlock/unlock` | many readers OR one writer |
| `pthread_barrier_wait` | wait until N threads reach the same point |
| `sem_init/wait/trywait/post/destroy` | counting semaphore |
| `pthread_once(&once, fn)` | one-time initialisation |
| `pthread_key_create/setspecific/getspecific` | per-thread data with destructor |
| `pthread_sigmask` / `sigwait` / `pthread_kill` | per-thread signal mask / wait for signal / signal one thread |
| `atomic_*` (`<stdatomic.h>`) | lock-free operations on single variables |

`pthread_*` functions **return** an error number (0 = success) instead of setting `errno`. That's why `common.h` has `check(err, what)`.

Every function has a man page: `man pthread_create`, `man sem_wait`, and so on (install `manpages-posix-dev` if they're missing).

---

## Alternatives to pthreads

pthreads is the usual choice on Linux, but it isn't the only way to get threads in C.

### C11 standard threads: `<threads.h>`

Part of the C language standard since C11, so it is the closest replacement. [`../05_c11_threads`](../05_c11_threads/) has a full worked example and a guide to choosing between the two. The API maps almost one to one:

| pthreads | C11 `<threads.h>` |
|----------|-------------------|
| `pthread_t` | `thrd_t` |
| `pthread_create(&t, NULL, fn, arg)` | `thrd_create(&t, fn, arg)` |
| `void *fn(void *arg)` | `int fn(void *arg)` |
| `pthread_join(t, &ret)` | `thrd_join(t, &res)` (`res` is an `int`) |
| `pthread_detach(t)` / `pthread_exit(ret)` | `thrd_detach(t)` / `thrd_exit(res)` |
| `pthread_mutex_t` | `mtx_t` (`mtx_init`, `mtx_lock`, `mtx_unlock`) |
| `pthread_cond_t` | `cnd_t` (`cnd_wait`, `cnd_signal`, `cnd_broadcast`) |
| `pthread_key_t` | `tss_t` (`tss_create`, `tss_get`, `tss_set`) |
| `_Thread_local` | `thread_local` |
| `pthread_once` | `call_once` |
| return value 0 = success | return value `thrd_success` |

Example 01 rewritten with C11 threads (several arguments still go in a struct):

```c
#include <stdio.h>
#include <threads.h>

struct job {
	int id;
	long from, to;
	long result;
};

static int sum_range(void *p)	/* returns int, not void * */
{
	struct job *j = p;

	for (long i = j->from; i <= j->to; i++)
		j->result += i;
	return 0;
}

int main(void)
{
	thrd_t t;
	struct job j = { .id = 1, .from = 1, .to = 100 };

	if (thrd_create(&t, sum_range, &j) != thrd_success)
		return 1;
	thrd_join(t, NULL);
	printf("sum = %ld\n", j.result);	/* 5050 */
	return 0;
}
```

```sh
gcc -std=c11 -Wall c11_threads.c -o c11_threads -pthread
```

- **Pros:** standard and portable, including Windows (MSVC 2022 and later).
- **Cons:** fewer features: no thread attributes (stack size, created-detached), no read-write locks, no barriers, no per-thread signal masks. Needs glibc 2.28 or later, and on Linux it is a thin wrapper around pthreads anyway.

### OpenMP

You mark loops with compiler directives and the compiler creates and joins the threads:

```c
long sum = 0;
#pragma omp parallel for reduction(+:sum)
for (long i = 1; i <= 1000000; i++)
	sum += i;
```

```sh
gcc -fopenmp omp_sum.c -o omp_sum
```

Excellent for number-crunching loops. Poor for long-lived workers, servers or producer/consumer designs.

### Linux `clone()`

The raw system call that pthreads itself uses. You allocate the stack yourself and pass flags such as `CLONE_VM | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD`. Good for learning what a thread really is. Avoid it in real code: libc (`errno`, `malloc`, stdio locks) assumes threads were created by pthreads.

### Platform APIs

| Platform | API |
|----------|-----|
| Windows | `_beginthreadex()` (or `CreateThread()`), `WaitForSingleObject()`, `CRITICAL_SECTION`, `SRWLOCK` |
| macOS / iOS | Grand Central Dispatch: `dispatch_async()` on task queues instead of raw threads |

### Cross-platform libraries

These use pthreads on Linux and native threads on Windows:

| Library | Thread API |
|---------|------------|
| GLib | `g_thread_new`, `GMutex`, `GCond`, `GThreadPool` |
| SDL | `SDL_CreateThread`, `SDL_mutex` |
| libuv | `uv_thread_create`, `uv_mutex_t`, built-in thread pool |
| tinycthread | single-file `<threads.h>` for compilers that lack C11 threads |

### Not quite threads

- **`fork()`**: separate processes. No shared memory unless you set it up, so they talk through IPC (`../01_message_queue`, `../02_signals`).
- **Coroutines** (`ucontext`, libco, libdill): many tasks take turns on one thread. Cheap switching, but no true parallelism.

### Which one?

| Situation | Use |
|-----------|-----|
| Linux/POSIX only, need every feature | pthreads |
| Portable C, basic threads and locks are enough | C11 `<threads.h>` |
| Parallelise a heavy loop | OpenMP |
| Windows-only code | `_beginthreadex` |
| Already using GLib / SDL / libuv | that library's threads |

---

## Inspecting threads from the shell

```sh
ps -eLf | grep 03_condvar        # one line per thread (LWP column = thread ID)
top -H -p <pid>                  # per-thread CPU usage
ls /proc/<pid>/task/             # one directory per thread
gdb -p <pid>  →  info threads, thread 2, bt
```

The examples finish in well under a second. To have time to look at them, raise a `msleep()` value or put a `sleep(30);` in a thread function.

---

## Common pitfalls

- **Forgetting to lock for reads.** Reading shared data while another thread writes it is a race too.
- **Deadlock.** Thread A locks `m1` then `m2`, thread B locks `m2` then `m1`, and both wait forever. Always take locks in one fixed global order.
- **`if` instead of `while` around `pthread_cond_wait`.**
- **Passing a pointer to a loop variable or a stack variable that goes out of scope** as the thread argument.
- **Returning from `main` while threads still run.** They are killed mid-work.
- **Neither joining nor detaching.** Each such thread leaks its stack (about 8 MB of address space by default).
- **Calling non-thread-safe functions** (`strtok`, `rand`, `localtime`) from several threads. Use the `_r` versions (`strtok_r`, `localtime_r`).
- **Using `volatile` for synchronisation.** It does not make anything atomic. Example 02 uses it only to keep the race visible.
- **`fork()` in a threaded program.** Only the calling thread is copied. Locks held by other threads stay locked forever in the child. Call `exec` right after.
- **Assuming an order.** "Thread 0 was created first, so it finishes first" is never guaranteed. If order matters, enforce it with join, a condition variable or a semaphore.

---

## Exercises

1. In 02, make each thread count into a local variable and add it to the shared counter once at the end. Compare the time against the mutex and atomic versions.
2. In 03, replace `pthread_cond_signal` with `pthread_cond_broadcast`. Does it still work? Why is it slower?
3. Write a deadlock on purpose with two mutexes and two threads, watch it hang in `gdb` (`info threads`), then fix it with lock ordering.
4. Use `pthread_barrier_t` to make 4 threads all print "phase 1", then all print "phase 2", never mixed.
5. Build a small thread pool: N workers pull function pointers from the bounded queue in 03.
6. Change 06 so `main` waits for signals instead of sending them, then use `kill -HUP <pid>` and `kill <pid>` from another terminal. What happens if `main` doesn't block the signals before creating the threads?
