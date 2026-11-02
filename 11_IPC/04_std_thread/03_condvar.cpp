// SPDX-License-Identifier: GPL-2.0
/*
 * 03_condvar - producer/consumer over a bounded buffer.
 *
 * The C++ version of ../03_threads/03_condvar.c. Read that file's header
 * for how a condition variable works and why the wait needs a loop.
 *
 *   pthreads (C)                         C++
 *   ------------------------------------ ---------------------------------
 *   pthread_cond_t cv = ..._INITIALIZER  std::condition_variable cv;
 *   pthread_mutex_lock(&m)               std::unique_lock<std::mutex> lk(m);
 *   while (!cond)                        cv.wait(lk, [&]{ return cond; });
 *       pthread_cond_wait(&cv, &m);      (the predicate form has the
 *                                         while loop built in)
 *   pthread_cond_signal / broadcast      cv.notify_one() / notify_all()
 *
 * WHY unique_lock AND NOT lock_guard
 * wait() must UNLOCK the mutex while it sleeps and lock it again when it
 * wakes. lock_guard can't be unlocked early; unique_lock can, so wait()
 * requires a unique_lock.
 *
 * The C version used global variables and STOP = -1. Here the buffer is
 * a CLASS TEMPLATE, BoundedQueue<T, Cap>, so it works for any item type
 * and capacity (see ../../12_templates/02_class_template.cpp). Instead
 * of a poison pill, close() wakes everybody and pop() then returns
 * std::nullopt ("no more items").
 */
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

/*
 * A thread-safe ring buffer holding at most Cap items of type T.
 * Every member is private and every public function locks the mutex,
 * so callers can't touch the data without the lock: the class enforces
 * the rule that the C version only stated in a comment.
 */
template <typename T, std::size_t Cap>
class BoundedQueue {
public:
	/* Add v, sleeping while the queue is full. */
	void push(T v)
	{
		std::unique_lock lk(m);

		/*
		 * wait(lock, predicate) is exactly:
		 *     while (!predicate()) wait(lock);
		 * so spurious and stolen wakeups are handled for us.
		 */
		not_full.wait(lk, [this] { return count < Cap; });

		buf[tail] = std::move(v);
		tail = (tail + 1) % Cap;
		count++;

		/*
		 * Unlock BEFORE notifying: the woken consumer can grab the
		 * mutex at once instead of waking up only to block on it.
		 * (Notifying while still locked is also correct, just slower.)
		 */
		lk.unlock();
		not_empty.notify_one();
	}

	/*
	 * Remove and return one item, sleeping while the queue is empty.
	 * Returns std::nullopt once the queue is closed AND drained.
	 */
	std::optional<T> pop()
	{
		std::unique_lock lk(m);

		not_empty.wait(lk, [this] { return count > 0 || closed; });
		if (count == 0)
			return std::nullopt;	/* closed and empty: we're done */

		T v = std::move(buf[head]);
		head = (head + 1) % Cap;
		count--;

		lk.unlock();
		not_full.notify_one();
		return v;
	}

	/*
	 * No more items will be pushed. notify_all wakes EVERY sleeping
	 * consumer so each can see "closed" and return. notify_one would
	 * leave the others asleep forever.
	 */
	void close()
	{
		{
			std::lock_guard lk(m);	/* change state under the lock */
			closed = true;
		}
		not_empty.notify_all();
	}

private:
	std::array<T, Cap> buf{};
	std::size_t head = 0, tail = 0, count = 0;
	bool closed = false;

	std::mutex m;
	std::condition_variable not_empty;	/* consumers wait here */
	std::condition_variable not_full;	/* producers wait here */
};

int main()
{
	constexpr int producers = 2, consumers = 3, items = 10;
	constexpr std::size_t cap = 4;	/* small, so producers do block */

	BoundedQueue<int, cap> q;
	std::vector<long> sums(consumers);	/* one slot per consumer */

	std::printf("%d producers x %d items -> buffer[%zu] -> %d consumers\n",
		    producers, items, cap, consumers);

	/*
	 * jthreads join themselves when they're destroyed. We put them in
	 * two vectors so we can control WHEN that happens: clearing a
	 * vector joins every thread in it.
	 */
	std::vector<std::jthread> cons, prod;

	for (int c = 0; c < consumers; c++)
		cons.emplace_back([&q, &sum = sums[c]] {
			int n = 0;

			/* Loop until pop() says the queue is closed. */
			while (std::optional<int> v = q.pop()) {
				sum += *v;
				n++;
			}
			std::printf("  consumer got %2d items, sum %ld\n", n, sum);
		});

	for (int p = 1; p <= producers; p++)
		prod.emplace_back([&q, p] {	/* p captured BY VALUE */
			for (int i = 1; i <= items; i++) {
				q.push(p * 100 + i);
				std::this_thread::sleep_for(5ms);
			}
		});

	prod.clear();	/* join all producers: nothing more will be pushed */
	q.close();	/* wake the consumers so they can finish */
	cons.clear();	/* join all consumers */

	long total = 0, expect = 0;
	for (long s : sums)
		total += s;
	for (int p = 1; p <= producers; p++)
		for (int i = 1; i <= items; i++)
			expect += p * 100 + i;
	std::printf("  total %ld, expected %ld: %s\n", total, expect,
		    total == expect ? "every item delivered exactly once" : "BUG");
	return total == expect ? 0 : 1;
}
