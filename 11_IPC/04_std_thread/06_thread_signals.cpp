// SPDX-License-Identifier: GPL-2.0
/*
 * 06_thread_signals - signals + std::jthread + cooperative stopping.
 *
 * The C++ version of ../03_threads/06_thread_signals.c. Read that file's
 * header for why one dedicated sigwait() thread is the clean pattern.
 *
 * WHAT C++ GIVES YOU, AND WHAT IT DOESN'T
 *   - The standard library has NO thread signal API: no sigmask, no
 *     sigwait. Signals are POSIX, so we still call pthread_sigmask()
 *     and sigwait(). That works because std::thread is a pthread on
 *     Linux, and new std::threads inherit the creator's mask just the
 *     same.
 *   - What C++20 does add is a standard way to ask a thread to stop:
 *
 *     std::jthread t([](std::stop_token st) {
 *             while (!st.stop_requested()) ...work...
 *     });
 *     t.request_stop();   // sets the flag that st sees
 *
 *     This replaces the hand-made "atomic_bool stop" of the C version.
 *     And jthread's destructor calls request_stop() then join(), so
 *     stopping and joining can't be forgotten.
 *
 *   - std::condition_variable_any can wait on a stop_token, so a thread
 *     asleep in wait() wakes up AT ONCE when a stop is requested,
 *     instead of only checking the flag after each sleep.
 */
#include <signal.h>	/* sigset_t, pthread_sigmask, sigwait, kill */
#include <unistd.h>	/* getpid */

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <stop_token>
#include <system_error>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

/* Turn a pthread-style error number into a C++ exception. */
static void check(int err, const char *what)
{
	if (err)
		throw std::system_error(err, std::generic_category(), what);
}

int main()
{
	constexpr int nworkers = 2;
	int reloads = 0;	/* only the signal thread touches it */

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);

	/*
	 * STEP 1: block the signals in main BEFORE creating any thread.
	 * Every std::thread / std::jthread created after this inherits it.
	 */
	check(pthread_sigmask(SIG_BLOCK, &set, nullptr), "pthread_sigmask");

	/*
	 * Workers: each lambda takes a std::stop_token as its FIRST
	 * parameter. jthread sees that and passes in a token linked to
	 * this thread's own stop request.
	 */
	std::mutex m;
	std::condition_variable_any cv;	/* "_any" can wait on a stop_token */
	std::vector<std::jthread> workers;

	for (int id = 0; id < nworkers; id++)
		workers.emplace_back([&m, &cv, id](std::stop_token st) {
			int ticks = 0;
			std::unique_lock lk(m);

			/*
			 * wait_for(lock, token, time, predicate) sleeps up to
			 * 20ms (one tick of "work"), but returns IMMEDIATELY
			 * if stop is requested. It returns the predicate's
			 * value, so the predicate itself must check the token:
			 * true means "stop was requested", false "timed out".
			 */
			while (!cv.wait_for(lk, st, 20ms,
					    [&st] { return st.stop_requested(); }))
				ticks++;
			std::printf("  worker %d: stopped cleanly after %d ticks\n",
				    id, ticks);
		});

	/* STEP 2: the only thread that deals with signals. */
	std::jthread sig_thread([&set, &reloads, &workers] {
		for (;;) {
			int sig;

			check(sigwait(&set, &sig), "sigwait");
			if (sig == SIGHUP) {
				reloads++;
				std::puts("  signal thread: SIGHUP -> reload config");
				continue;
			}
			std::printf("  signal thread: %s -> asking workers to stop\n",
				    sig == SIGTERM ? "SIGTERM" : "SIGINT");
			/* The C++20 replacement for "atomic_store(&stop, 1)". */
			for (auto &w : workers)
				w.request_stop();
			return;
		}
	});

	/* Simulate "kill -HUP <pid>" twice, then "kill <pid>". */
	std::puts("main: kill(getpid(), SIGHUP) twice, then SIGTERM");
	std::this_thread::sleep_for(50ms);
	kill(getpid(), SIGHUP);
	std::this_thread::sleep_for(50ms);
	kill(getpid(), SIGHUP);
	std::this_thread::sleep_for(50ms);
	kill(getpid(), SIGTERM);

	/* STEP 3: orderly shutdown. join() explicitly so we can print after. */
	sig_thread.join();
	for (auto &w : workers)
		w.join();
	std::printf("main: all threads joined, %d reload(s) handled\n", reloads);
	return 0;
}
