// SPDX-License-Identifier: GPL-2.0
/*
 * 02_race_mutex - a data race, and two ways to fix it.
 *
 * The C++ version of ../03_threads/02_race_mutex.c. Read that file's
 * header for why "counter++" loses updates (load / add / store).
 *
 *   pthreads (C)                         C++
 *   ------------------------------------ ---------------------------------
 *   pthread_mutex_t m = ..._INITIALIZER  std::mutex m;  (no init needed)
 *   pthread_mutex_lock / unlock          std::lock_guard<std::mutex> g(m);
 *                                        locks now, unlocks automatically
 *                                        at the end of the scope (RAII)
 *   atomic_long  (<stdatomic.h>)         std::atomic<long>  (<atomic>)
 *   atomic_fetch_add(&c, 1)              c++  or  c.fetch_add(1)
 *
 * WHY RAII LOCKING MATTERS
 * With lock()/unlock() it's easy to forget the unlock on an early return,
 * or to skip it when an exception is thrown, and the next thread then
 * waits forever. A lock_guard's destructor ALWAYS runs when the scope
 * ends, however it ends, so the mutex is always released.
 *
 * The unsafe version below is undefined behaviour on purpose, to show
 * the bug. Never write it in real code.
 */
#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

constexpr int nthreads = 4;
constexpr long loops = 1000000;

/*
 * volatile only stops the optimiser from folding the loop into one big
 * add, which would hide the race. It does NOT make anything thread-safe.
 */
static volatile long racy;

static long locked;
static std::mutex lock;		/* ready to use, unlike pthread_mutex_t */

static std::atomic<long> atomic_counter{0};

/* WRONG: no protection at all, so increments get lost. */
static void inc_racy()
{
	for (long i = 0; i < loops; i++)
		racy = racy + 1;	/* load / add / store; threads collide */
}

/* RIGHT: a mutex makes the increment one step for other threads. */
static void inc_locked()
{
	for (long i = 0; i < loops; i++) {
		/*
		 * The constructor locks (sleeping if another thread holds it);
		 * the destructor unlocks at the closing brace.
		 *
		 * C++17 can deduce the template argument, so this is the same
		 * as std::lock_guard<std::mutex> g(lock). For several mutexes
		 * at once use std::scoped_lock g(m1, m2), which locks them in
		 * a deadlock-free way.
		 */
		std::lock_guard g(lock);
		locked++;	/* critical section: one thread at a time */
	}
}

/* RIGHT and cheaper: one atomic CPU instruction per increment. */
static void inc_atomic()
{
	for (long i = 0; i < loops; i++)
		atomic_counter++;	/* overloaded ++ is an atomic fetch_add */
}

/*
 * Start nthreads threads running fn, wait for them, then compare the
 * counter (read through get) with the value it should have.
 *
 * fn and get are template parameters, so any callable works: function
 * pointer or lambda (see ../../12_templates).
 */
template <typename Fn, typename Get>
static void run(const char *name, Fn fn, Get get)
{
	auto t0 = std::chrono::steady_clock::now();
	std::vector<std::thread> threads;

	for (int i = 0; i < nthreads; i++)
		threads.emplace_back(fn);
	for (auto &t : threads)
		t.join();

	/* All threads have finished, so reading the counter is safe now. */
	long got = get();
	std::chrono::duration<double, std::milli> ms =
		std::chrono::steady_clock::now() - t0;
	std::printf("  %-7s %8ld / %ld  lost %7ld  %6.1f ms\n", name, got,
		    nthreads * loops, nthreads * loops - got, ms.count());
}

int main()
{
	std::printf("%d threads each increment a shared counter %ld times\n",
		    nthreads, loops);
	run("racy", inc_racy, [] { return static_cast<long>(racy); });
	run("mutex", inc_locked, [] { return locked; });
	run("atomic", inc_atomic, [] { return atomic_counter.load(); });

	std::puts("racy loses updates (the exact number changes every run);");
	std::puts("mutex and atomic are always exact; atomic is cheaper.");
	return 0;
}
