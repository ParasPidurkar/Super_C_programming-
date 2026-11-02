// SPDX-License-Identifier: GPL-2.0
/*
 * 01_create_join - start threads, pass them arguments, get results back.
 *
 * The C++ version of ../03_threads/01_create_join.c. Same ideas, but
 * the C++ standard library (<thread>) does the plumbing for you:
 *
 *   pthreads (C)                         C++ <thread>
 *   ------------------------------------ ---------------------------------
 *   pthread_t tid;                       std::thread t;
 *   pthread_create(&tid, NULL, fn, arg)  std::thread t(fn, arg1, arg2...);
 *   void *fn(void *arg)                  ANY callable: function, lambda,
 *                                        functor, member function
 *   pack args into a struct              pass as many args as you like
 *   pthread_join(tid, &ret)              t.join()
 *   pthread_detach(tid)                  t.detach()
 *   return a malloc'd void *             std::async + std::future<T>
 *   check(err) on every call             errors are thrown as
 *                                        std::system_error
 *
 * On Linux, std::thread is a thin wrapper around pthreads, so the
 * behaviour (scheduling, cost, sharing) is identical. You still build
 * with -pthread.
 *
 * THE ONE BIG RULE: a std::thread object that is still "joinable" when
 * it is destroyed calls std::terminate() and kills the program. So
 * every std::thread must be join()ed or detach()ed before it goes out
 * of scope. C++20's std::jthread (part 5) does the join for you.
 */
#include <chrono>
#include <cstdio>
#include <functional>	/* std::ref */
#include <future>	/* std::async, std::future */
#include <numeric>
#include <thread>
#include <vector>

using namespace std::chrono_literals;	/* lets us write 50ms */

/*
 * A thread function can take any number of arguments of any type.
 * No void * and no casting back, unlike pthreads.
 */
static void sum_range(int id, long from, long to, long &result)
{
	long s = 0;	/* lives on THIS thread's own stack */

	for (long i = from; i <= to; i++)
		s += i;

	/*
	 * Safe without a lock: each thread writes a DIFFERENT element, and
	 * main only reads it after join(), which makes the write visible.
	 */
	result = s;
	std::printf("  thread %d: sum(%ld..%ld) = %ld\n", id, from, to, s);
}

/* Returns a value normally. std::async (part 3) hands it back to us. */
static long square(int n)
{
	return static_cast<long>(n) * n;
}

int main()
{
	constexpr int nthreads = 4;
	constexpr long per = 1000000 / nthreads;

	/* ---- Part 1: split one big job across several threads ---- */
	std::printf("1. split sum(1..1000000) across %d threads\n", nthreads);

	std::vector<std::thread> threads;
	std::vector<long> results(nthreads);	/* one output slot PER thread */

	for (int i = 0; i < nthreads; i++) {
		/*
		 * std::thread(function, args...) starts the thread at once.
		 *
		 * ARGUMENTS ARE COPIED into the new thread. That makes passing
		 * i safe (each thread gets its own copy of i's CURRENT value),
		 * which fixes the classic "&i" bug of the C version.
		 *
		 * Because args are copied, a reference parameter (long &) would
		 * bind to the copy, not to results[i]. std::ref() says "really
		 * pass a reference". Without it this line doesn't even compile.
		 *
		 * emplace_back builds the std::thread in place in the vector.
		 * std::thread can be MOVED but not copied: one thread, one owner.
		 */
		threads.emplace_back(sum_range, i, i * per + 1, (i + 1) * per,
				     std::ref(results[i]));
	}

	/* Wait for every thread. They may FINISH in any order. */
	for (auto &t : threads)
		t.join();

	long total = std::accumulate(results.begin(), results.end(), 0L);
	std::printf("  total = %ld (expected %ld)\n", total,
		    1000000L * 1000001L / 2);

	/* ---- Part 2: a lambda as the thread function ---- */
	std::puts("2. a lambda thread, capturing by reference");
	int counter = 0;
	/*
	 * [&counter] captures counter BY REFERENCE: the thread changes
	 * main's variable. Safe here only because main does nothing with
	 * counter until join() returns. Capturing locals by reference in a
	 * thread that outlives them is the C++ form of the dangling pointer
	 * bug, so be careful with [&] in detached threads.
	 */
	std::thread lam([&counter] {
		for (int i = 0; i < 10; i++)
			counter++;
		std::printf("  lambda thread id: %zu\n",
			    std::hash<std::thread::id>{}(std::this_thread::get_id()));
	});
	lam.join();
	std::printf("  counter = %d\n", counter);

	/* ---- Part 3: getting a return value with std::async ---- */
	std::puts("3. get a return value with std::async + std::future");
	/*
	 * std::thread throws away the function's return value. To get one
	 * back, use std::async: it runs the function and gives you a
	 * std::future<long>, a "box the result will arrive in".
	 *
	 * std::launch::async forces a NEW thread. Without it the library may
	 * defer the call and run it lazily inside get().
	 *
	 * Compare with C: no malloc for the result, no void * cast, no free.
	 * And if square() throws, get() re-throws the exception in main.
	 */
	std::future<long> fut = std::async(std::launch::async, square, 7);
	std::printf("  square(7) = %ld\n", fut.get());	/* waits, then returns */

	/* ---- Part 4: detached threads ---- */
	std::puts("4. a detached thread cleans itself up; it can't be joined");
	std::thread bg([] {
		std::this_thread::sleep_for(50ms);	/* pretend to work */
		std::puts("  detached thread: finished, nobody will join me");
	});
	/*
	 * After detach() the std::thread object no longer owns anything, so
	 * bg can be destroyed safely. The thread itself keeps running. Use
	 * this only for fire-and-forget work, and capture nothing by
	 * reference that could die first.
	 */
	bg.detach();

	/* ---- Part 5: std::jthread joins automatically (C++20) ---- */
	std::puts("5. std::jthread joins itself in its destructor (RAII)");
	{
		std::jthread jt([] { std::puts("  jthread: running"); });
		/*
		 * No join() needed: when jt goes out of scope at the closing
		 * brace, its destructor joins. So an early return or an
		 * exception can't leave a joinable thread behind. Prefer
		 * std::jthread in new code. (It can also be asked to stop;
		 * see example 06.)
		 */
	}
	std::puts("  jthread: joined by its destructor");

	std::printf("this machine runs %u threads in parallel\n",
		    std::thread::hardware_concurrency());

	/*
	 * Returning from main ends the WHOLE process, killing the detached
	 * thread mid-work. Give it time to finish. (Real code would wait on
	 * a latch or semaphore, as example 04 does.)
	 */
	std::this_thread::sleep_for(100ms);
	return 0;
}
