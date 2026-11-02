// SPDX-License-Identifier: GPL-2.0
/*
 * 04_semaphore - limit how many threads do something at once.
 *
 * The C++20 version of ../03_threads/04_semaphore.c. Read that file's
 * header for what a semaphore is (a bowl of N tokens).
 *
 *   POSIX (C)                            C++20
 *   ------------------------------------ ---------------------------------
 *   sem_t s;  sem_init(&s, 0, N)         std::counting_semaphore<MAX> s(N);
 *   sem_wait(&s)                         s.acquire()
 *   sem_post(&s)                         s.release()
 *   sem_trywait(&s)                      s.try_acquire()
 *   sem_destroy(&s)                      (destructor, automatic)
 *   "done" semaphore initialised to 0    std::latch done(N);
 *
 * MAX is a template parameter: the largest count the semaphore must
 * ever hold. std::binary_semaphore is counting_semaphore<1>.
 *
 * std::latch is a one-shot countdown, built for "wait until N things
 * have happened": workers call count_down(), main calls wait(), which
 * returns once the count reaches zero. It can't be reset; for a
 * reusable version (every round), see std::barrier.
 *
 * No EINTR loops are needed: unlike sem_wait(), acquire() and wait()
 * don't return early when a signal arrives.
 */
#include <atomic>
#include <chrono>
#include <cstdio>
#include <latch>
#include <semaphore>
#include <thread>

using namespace std::chrono_literals;

constexpr int workers = 6;
constexpr int slots_n = 2;	/* at most this many workers inside at once */

static std::counting_semaphore<slots_n> slots(slots_n);	/* free slots */
static std::latch done(workers);	/* counts down as workers finish */

static std::atomic<int> inside, max_inside;	/* to PROVE the limit */

static void worker(int id)
{
	/* Take a slot, sleeping if all slots_n are in use. */
	slots.acquire();

	/* ---- at most slots_n threads can be in this part at a time ---- */
	int n = ++inside;	/* atomic ++ returns the NEW value */

	/*
	 * max_inside = max(max_inside, n), done atomically. If another
	 * thread changed max_inside first, compare_exchange_weak reloads m
	 * with the current value and we try again.
	 */
	int m = max_inside.load();
	while (n > m && !max_inside.compare_exchange_weak(m, n))
		;

	std::printf("  worker %d in  (%d inside)\n", id, n);
	std::this_thread::sleep_for(30ms);	/* the "limited" work */
	--inside;
	std::printf("  worker %d out\n", id);

	slots.release();	/* give the slot back, waking one waiter */
	/* ---- end of limited section ---- */

	done.count_down();	/* tell main "one more worker finished" */
}

int main()
{
	std::printf("%d workers, %d slots\n", workers, slots_n);

	/*
	 * Detached, so we can't join them; the latch is how main knows
	 * they're done. (With jthreads you wouldn't need the latch, but
	 * this mirrors the C version.)
	 */
	for (int i = 0; i < workers; i++)
		std::thread(worker, i).detach();

	done.wait();	/* sleeps until all workers have counted down */

	std::printf("  never more than %d inside at once (limit %d)\n",
		    max_inside.load(), slots_n);
	return max_inside <= slots_n ? 0 : 1;
}
