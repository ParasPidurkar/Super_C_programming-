# Threads with the C++ standard library (`<thread>`)

These are the same six lessons as [`../03_threads`](../03_threads/), written with the C++20 thread library instead of raw pthreads. Read the C version first. Its README explains the concepts (races, condition variables, semaphores, signal masks). This folder shows how C++ makes them shorter and safer.

On Linux `std::thread` **is** a pthread underneath, so threads run, share memory and cost exactly the same. What changes is the interface:

- **RAII.** Locks unlock themselves, `jthread` joins itself, and `thread_local` objects destroy themselves.
- **Templates.** Pass any callable with any arguments. No `void *` and no casts.
- **Exceptions.** Errors throw `std::system_error`, so there's no `check(err)` after every call.

## Build and run

You need g++ 11+ or clang 14+ (for C++20 `<semaphore>`, `<latch>` and `jthread`).

```sh
cd 11_IPC/04_std_thread

make            # build everything
make run        # run examples 01-06 one after another
make check      # run them under AddressSanitizer + UBSan
make tsan       # run the race-free ones under ThreadSanitizer
make clean
```

Flags are as strict as in the C folder (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`) plus `-std=c++20 -pthread`.

| File | Teaches |
|------|---------|
| `01_create_join.cpp` | `std::thread`, args copied vs `std::ref`, lambdas, `join`/`detach`, return values with `std::async` + `std::future`, `std::jthread` |
| `02_race_mutex.cpp` | the same race, fixed with `std::mutex` + `std::lock_guard` and with `std::atomic<long>` |
| `03_condvar.cpp` | `std::condition_variable`, `std::unique_lock`, `wait(lock, predicate)`, a `BoundedQueue<T, Cap>` **class template**, shutting down with `close()` + `std::optional` instead of a poison pill |
| `04_semaphore.cpp` | `std::counting_semaphore`, `std::latch` |
| `05_tls_once.cpp` | `thread_local` objects with destructors, `std::call_once`, thread-safe function-local `static` |
| `06_thread_signals.cpp` | `pthread_sigmask` + `sigwait` with `std::jthread`, `std::stop_token`, `condition_variable_any` that wakes on stop |

To build and run **just one** example:

```sh
make 03_condvar && ./03_condvar

# or by hand:
g++ -std=c++20 -Wall -pthread 03_condvar.cpp -o 03_condvar
./03_condvar
```

## The examples, one by one

Each program does the same job as its C twin and prints almost the same output, so you can compare them side by side. Below, each section covers **what is new in C++**. For the concepts themselves (why races happen, why the wait needs a loop and so on) see the [C README](../03_threads/README.md#the-examples-one-by-one).

> Your output order and counts will differ from run to run. That's normal for threads. The final totals are always the same.

### 01 — creating and joining threads

Five small parts:

1. **`std::thread` with arguments.** `threads.emplace_back(sum_range, i, from, to, std::ref(results[i]))` starts a thread running `sum_range(i, from, to, results[i])`. There's no struct and no `void *`: pass as many typed arguments as you like.
   - Arguments are **copied** into the thread, so passing `i` is safe (this fixes the C `&i` bug).
   - For a reference parameter (`long &result`) you must write `std::ref(...)`, otherwise it won't compile.
2. **A lambda as the thread body.** `std::thread lam([&counter] { ... });` captures `counter` by reference. That's safe here only because `main` joins before using it.
3. **Getting a return value.** `std::async(std::launch::async, square, 7)` runs `square(7)` on a new thread and returns a `std::future<long>`. `fut.get()` waits and gives you `49`. No `malloc`, no cast, no `free`. If the function throws, `get()` re-throws the exception in `main`.
4. **Detached thread.** `bg.detach()`, the same idea as in C.
5. **`std::jthread`** (C++20) joins itself when it goes out of scope, so you can't forget.

```
1. split sum(1..1000000) across 4 threads
  thread 1: sum(250001..500000) = 93750125000
  thread 0: sum(1..250000) = 31250125000
  thread 3: sum(750001..1000000) = 218750125000
  thread 2: sum(500001..750000) = 156250125000
  total = 500000500000 (expected 500000500000)
2. a lambda thread, capturing by reference
  lambda thread id: 6770828547147873685
  counter = 10
3. get a return value with std::async + std::future
  square(7) = 49
4. a detached thread cleans itself up; it can't be joined
5. std::jthread joins itself in its destructor (RAII)
  jthread: running
  jthread: joined by its destructor
this machine runs 16 threads in parallel
  detached thread: finished, nobody will join me
```

The detached thread's message comes **last**, after part 5, because nobody waits for it: it prints whenever it finishes. `hardware_concurrency()` reports how many threads your CPU can run truly in parallel (16 here; yours may differ).

**The one big rule:** if a `std::thread` object is destroyed while still joinable (neither joined nor detached), the program calls `std::terminate()` and dies. `std::jthread` avoids this.

### 02 — data races, mutexes and atomics

The same experiment as in C: 4 threads × 1,000,000 increments, done three ways.

| C | C++ |
|---|-----|
| `pthread_mutex_lock(&lock); locked++; pthread_mutex_unlock(&lock);` | `{ std::lock_guard g(lock); locked++; }` |
| `atomic_long c; atomic_fetch_add(&c, 1);` | `std::atomic<long> c; c++;` |

`std::lock_guard` locks in its constructor and unlocks in its destructor (**RAII**). Even if the code returns early or throws, the mutex is always released. For locking several mutexes at once without deadlock, use `std::scoped_lock g(m1, m2);`.

```
4 threads each increment a shared counter 1000000 times
  racy     1010176 / 4000000  lost 2989824     6.2 ms
  mutex    4000000 / 4000000  lost       0   254.0 ms
  atomic   4000000 / 4000000  lost       0    37.6 ms
```

The results and timings match the C version almost exactly, because underneath it's the same pthread mutex and the same CPU instruction.

### 03 — condition variables

The bounded buffer becomes a reusable **class template**, `BoundedQueue<T, Cap>` (here `BoundedQueue<int, 4>`):

- `push(v)` waits while the queue is full, then adds `v`.
- `pop()` waits while it's empty, then returns an item. It returns `std::nullopt` once the queue is closed and drained.
- `close()` says "no more items" and wakes **all** waiting consumers with `notify_all()`.

Key C++ pieces:

```cpp
std::unique_lock lk(m);
not_empty.wait(lk, [this] { return count > 0 || closed; });
```

- `wait(lock, predicate)` is exactly `while (!predicate()) wait(lock);`, the loop from C built in.
- It needs `std::unique_lock`, not `lock_guard`, because `wait` must unlock while sleeping.
- The mutex and data are `private`, so nobody can touch the buffer without the lock. The class enforces the rule.

**Stopping without a poison pill.** The consumers loop `while (std::optional<int> v = q.pop())`. After the producers finish, `main` calls `q.close()`. Each consumer drains what's left, then `pop()` returns empty and the loop ends.

The threads are `std::jthread`s in two vectors. `prod.clear()` destroys (and so joins) all producers, then `q.close()`, then `cons.clear()` joins all consumers.

```
2 producers x 10 items -> buffer[4] -> 3 consumers
  consumer got  6 items, sum 933
  consumer got  7 items, sum 1140
  consumer got  7 items, sum 1037
  total 3110, expected 3110: every item delivered exactly once
```

### 04 — semaphores

| C | C++20 |
|---|-------|
| `sem_t slots; sem_init(&slots, 0, 2);` | `std::counting_semaphore<2> slots(2);` |
| `sem_wait` / `sem_post` | `slots.acquire()` / `slots.release()` |
| semaphore at 0, `main` waits 6 times | `std::latch done(6);` workers call `done.count_down()`, `main` calls `done.wait()` once |

A `std::latch` is a one-shot countdown built exactly for "wait until N things have happened". It can't be reset; use `std::barrier` for something reusable.

```
6 workers, 2 slots
  worker 0 in  (1 inside)
  worker 1 in  (2 inside)
  worker 0 out
  worker 1 out
  worker 3 in  (1 inside)
  worker 4 in  (2 inside)
  ...
  never more than 2 inside at once (limit 2)
```

### 05 — thread-local storage and `call_once`

| C | C++ |
|---|-----|
| `_Thread_local int my_count;` | `thread_local int my_count;` |
| `pthread_key_create` + destructor + `setspecific`/`getspecific` | `static thread_local ThreadBuf buf;`: any type, its destructor runs automatically at thread exit |
| `pthread_once(&once, init)` | `std::call_once(flag, init)` |
| — | a function-local `static` is initialised exactly once and thread-safely (the `cfg` "config" in the code) |

The pthread key machinery from C collapses into one line: a `thread_local` object of a class with a destructor.

```
  init: runs exactly once
  config: loaded once
  thread 0: my_count = 1, buf = "buffer of thread 0", mode=fast
  destructor: freeing "buffer of thread 0"
  thread 1: my_count = 2, buf = "buffer of thread 1", mode=fast
  destructor: freeing "buffer of thread 1"
  thread 2: my_count = 3, buf = "buffer of thread 2", mode=fast
  destructor: freeing "buffer of thread 2"
  main: my_count = 0 (its own copy, untouched)
```

`init` and `config` each appear once, although all three threads asked for them. Every thread has its own `my_count` and its own buffer, and each buffer is destroyed when its thread ends.

### 06 — signals with `std::jthread` and `std::stop_token`

C++ has **no** signal-mask API, so the signal part stays POSIX: `pthread_sigmask` in `main` before creating threads, and one thread looping on `sigwait()`. This works because `std::thread` is a pthread on Linux.

What C++20 replaces is the hand-made `atomic_bool stop` flag:

```cpp
workers.emplace_back([&m, &cv, id](std::stop_token st) {   // jthread passes the token
        std::unique_lock lk(m);
        while (!cv.wait_for(lk, st, 20ms, [&st] { return st.stop_requested(); }))
                ticks++;                                    // one 20 ms "tick" of work
});
...
w.request_stop();                                           // from the signal thread
```

- If the lambda's first parameter is a `std::stop_token`, `jthread` passes one in automatically.
- `request_stop()` sets the flag the token sees.
- `std::condition_variable_any::wait_for(lock, token, time, pred)` sleeps up to 20 ms, but wakes **immediately** when a stop is requested. Without it, a worker would only notice the stop at the end of its current sleep.

```
main: kill(getpid(), SIGHUP) twice, then SIGTERM
  signal thread: SIGHUP -> reload config
  signal thread: SIGHUP -> reload config
  signal thread: SIGTERM -> asking workers to stop
  worker 1: stopped cleanly after 7 ticks
  worker 0: stopped cleanly after 7 ticks
main: all threads joined, 2 reload(s) handled
```

## pthreads → C++ cheat sheet

| pthreads / C | C++ | Header |
|---|---|---|
| `pthread_create(&t, NULL, fn, arg)` | `std::thread t(fn, a, b, c);` | `<thread>` |
| `pthread_join(t, NULL)` | `t.join()` | |
| `pthread_detach(t)` | `t.detach()` | |
| join on every exit path | `std::jthread` (joins in its destructor) | |
| `pthread_self()` | `std::this_thread::get_id()` | |
| `nanosleep` / `msleep(ms)` | `std::this_thread::sleep_for(50ms)` | `<chrono>` |
| `void *` return + `malloc` | `std::async(...)` → `std::future<T>::get()` | `<future>` |
| `pthread_mutex_t` + lock/unlock | `std::mutex` + `std::lock_guard` / `std::scoped_lock` | `<mutex>` |
| `pthread_rwlock_t` | `std::shared_mutex` + `std::shared_lock` | `<shared_mutex>` |
| `pthread_cond_t` | `std::condition_variable` (+ `std::unique_lock`) | `<condition_variable>` |
| `while (!c) pthread_cond_wait(...)` | `cv.wait(lk, [&]{ return c; })` | |
| `sem_t` | `std::counting_semaphore<N>` / `std::binary_semaphore` | `<semaphore>` |
| "done" semaphore at 0 | `std::latch` | `<latch>` |
| `pthread_barrier_t` | `std::barrier` | `<barrier>` |
| `_Thread_local` / `pthread_key_create` | `thread_local` (any type, destructor runs) | |
| `pthread_once` | `std::call_once` or a function-local `static` | `<mutex>` |
| `atomic_long`, `atomic_fetch_add` | `std::atomic<long>`, `++` / `fetch_add` | `<atomic>` |
| hand-made `atomic_bool stop` | `std::stop_token` / `request_stop()` | `<stop_token>` |
| `pthread_sigmask`, `sigwait` | **no C++ equivalent**, keep using POSIX | `<signal.h>` |

## C++-specific pitfalls

- **Destroying a joinable `std::thread` calls `std::terminate()`.** Join or detach every thread, or use `std::jthread`.
- **Arguments are copied.** A `T&` parameter needs `std::ref(x)`, otherwise it won't compile.
- **`[&]` lambdas in detached threads.** The captured locals can die before the thread uses them, the C++ form of a dangling pointer.
- **`std::async` without `std::launch::async`** may not start a thread at all. The call can be deferred until `get()`.
- **A `std::future` from `std::async` blocks in its destructor** until the task ends. `std::async(f);` without keeping the future runs `f` synchronously.
- **`lock_guard` cannot be used with `cv.wait()`.** The wait needs a `unique_lock`, because it must unlock while sleeping.
- **`notify_one` on shutdown** leaves the other waiters asleep. Use `notify_all` when the state changes for everyone (see `close()` in 03).
- **`std::cout << a << b` from several threads** can interleave mid-line. The examples use `printf` (each call is atomic in glibc), or use C++20 `std::osyncstream`.

## Exercises

1. In 01, make `sum_range` throw for one thread. What happens with `std::thread`? Then run it through `std::async` and catch the exception from `get()`.
2. In 02, replace `std::lock_guard` with explicit `lock.lock()` / `lock.unlock()` and add an early `return` inside the loop. Watch the program hang.
3. Turn `BoundedQueue` in 03 into a thread pool: workers pop `std::function<void()>` jobs and run them.
4. Use `std::barrier` to make 4 threads print "phase 1" then "phase 2", never mixed (same as exercise 4 in the C folder).
5. Add a `std::shared_mutex` to a `std::map` cache: many readers with `shared_lock`, one writer with `unique_lock`.
