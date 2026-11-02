// SPDX-License-Identifier: GPL-2.0
/*
 * server - a long-running program that reacts to signals the way real
 * daemons do. Run it, then poke it from the keyboard or another terminal.
 *
 *   Ctrl-C  / kill -INT  <pid>   graceful shutdown (press twice to force)
 *             kill -TERM <pid>   graceful shutdown (what systemd sends)
 *             kill -HUP  <pid>   reload the configuration
 *             kill -USR1 <pid>   print a status line
 *   Ctrl-Z  / kill -STOP <pid>   pause; `fg` or kill -CONT <pid> resumes
 *   Ctrl-\                       SIGQUIT: default action, quit at once
 *             kill -9    <pid>   SIGKILL: no cleanup possible
 */
#define _XOPEN_SOURCE 700

#include "common.h"

static volatile sig_atomic_t stop_requested, reload_requested;
static volatile sig_atomic_t status_requested, resumed;

static void on_stop(int sig)
{
	if (stop_requested) {
		/* Second request: give up on cleanup. _exit() is signal-safe. */
		say("\nforced exit\n");
		_exit(128 + sig);
	}
	stop_requested = sig;
}

static void on_hup(int sig)
{
	(void)sig;
	reload_requested = 1;
}

static void on_usr1(int sig)
{
	(void)sig;
	status_requested = 1;
}

static void on_cont(int sig)
{
	(void)sig;
	resumed = 1;
}

int main(void)
{
	struct timespec tick = { 1, 0 };
	int config_version = 1;
	long ticks = 0;

	setvbuf(stdout, NULL, _IOLBF, 0);

	/*
	 * No SA_RESTART: we want nanosleep() below to return early, so we
	 * react to a signal at once instead of after the current tick.
	 */
	install(SIGINT, on_stop, 0);
	install(SIGTERM, on_stop, 0);
	install(SIGHUP, on_hup, 0);
	install(SIGUSR1, on_usr1, 0);
	install(SIGCONT, on_cont, 0);

	printf("server running as pid %d. Try, from another terminal:\n"
	       "  kill -HUP %d     kill -USR1 %d     kill %d\n"
	       "or press Ctrl-C, Ctrl-Z (then fg), Ctrl-\\ here.\n",
	       (int)getpid(), (int)getpid(), (int)getpid(), (int)getpid());

	while (!stop_requested) {
		if (nanosleep(&tick, NULL) == 0)
			printf("  working... tick %ld\n", ++ticks);

		/* Act on what the handlers recorded, outside the handlers. */
		if (reload_requested) {
			reload_requested = 0;
			printf("  SIGHUP: reloaded config (version %d)\n",
			       ++config_version);
		}
		if (status_requested) {
			status_requested = 0;
			printf("  SIGUSR1: status: %ld ticks, config v%d\n",
			       ticks, config_version);
		}
		if (resumed) {
			resumed = 0;
			puts("  SIGCONT: resumed after being stopped");
		}
	}

	printf("\n  %s: finishing current work, saving state...\n",
	       strsignal(stop_requested));
	msleep(500);	/* pretend cleanup; Ctrl-C again now forces exit */
	puts("  clean shutdown done");
	return EXIT_SUCCESS;
}
