// SPDX-License-Identifier: GPL-2.0
/*
 * 05_tls_once - per-thread data and one-time initialisation.
 *
 * The C++ version of ../03_threads/05_tls_once.c.
 *
 *   C / pthreads                         C++
 *   ------------------------------------ ---------------------------------
 *   _Thread_local int x;                 thread_local int x;
 *   pthread_key_create(&k, destructor)   thread_local Object obj;
 *   + setspecific / getspecific          (the object's destructor runs
 *                                         automatically at thread exit)
 *   pthread_once(&once, init)            std::call_once(flag, init)
 *                                        or a function-local static
 *
 * C's _Thread_local only works for plain values, so per-thread buffers
 * needed a pthread key and a manual destructor. In C++, thread_local
 * works for ANY type: a std::string or a class gets constructed on
 * first use in each thread and destroyed when that thread exits.
 */
#include <cstdio>
#include <mutex>	/* std::once_flag, std::call_once */
#include <string>
#include <thread>
#include <vector>

/* One copy per thread, each starting at 0. No lock needed. */
static thread_local int my_count = 0;

/*
 * A class with a destructor, to SHOW when per-thread objects die.
 * This replaces pthread_key_create() + free_buf() from the C version.
 */
struct ThreadBuf {
	std::string text = "(empty)";

	~ThreadBuf()
	{
		std::printf("  destructor: freeing \"%s\"\n", text.c_str());
	}
};

/*
 * Each thread that calls this gets its OWN ThreadBuf, built on the first
 * call in that thread and destroyed automatically when the thread ends.
 */
static ThreadBuf &thread_buf()
{
	static thread_local ThreadBuf buf;
	return buf;
}

static std::once_flag once;

static void init()
{
	std::puts("  init: runs exactly once");
}

/*
 * Another way to "run once": a function-local static. Since C++11 the
 * compiler guarantees it is initialised exactly once, even when many
 * threads call the function at the same time (it inserts the locking).
 * This is the usual way to write a thread-safe lazy singleton.
 */
static const std::string &config()
{
	static const std::string cfg = [] {
		std::puts("  config: loaded once");
		return std::string("mode=fast");
	}();
	return cfg;
}

static void work(int id)
{
	/* Many threads call this; init() still runs only once. */
	std::call_once(once, init);

	for (int i = 0; i <= id; i++)
		my_count++;	/* this thread's own copy */

	ThreadBuf &b = thread_buf();
	b.text = "buffer of thread " + std::to_string(id);
	std::printf("  thread %d: my_count = %d, buf = \"%s\", %s\n", id,
		    my_count, b.text.c_str(), config().c_str());
	/* No cleanup here: b's destructor runs when this thread exits. */
}

int main()
{
	constexpr int nthreads = 3;

	/*
	 * Each thread runs and is joined before the next one starts, so the
	 * output is easy to follow. thread_local works the same when they
	 * run at the same time.
	 */
	for (int i = 0; i < nthreads; i++)
		std::thread(work, i).join();

	/* main is a thread too, with its own my_count, never incremented. */
	std::printf("  main: my_count = %d (its own copy, untouched)\n",
		    my_count);
	return 0;
}
