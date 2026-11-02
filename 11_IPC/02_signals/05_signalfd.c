// SPDX-License-Identifier: GPL-2.0
/*
 * 05_signalfd - the modern Linux way: read signals from a file descriptor.
 *
 * Handlers are awkward: they interrupt your code at any point and may
 * only call async-signal-safe functions. signalfd() avoids all that.
 * Block the signals, then read() them as ordinary data from an fd, which
 * fits straight into a poll()/epoll() event loop next to sockets and
 * pipes. No handler code runs at all.
 *
 * Here the parent polls two fds at once: a pipe the child writes text
 * to, and a signalfd. The child mixes messages and signals, then sends
 * SIGTERM to ask the parent to shut down.
 */
#define _GNU_SOURCE	/* signalfd() is Linux-only */

#include <poll.h>
#include <sys/signalfd.h>
#include <sys/wait.h>

#include "common.h"

static void child(int wfd)
{
	pid_t parent = getppid();

	msleep(100);
	kill(parent, SIGUSR1);
	msleep(100);
	if (write(wfd, "hello over the pipe", 20) < 0)
		_exit(EXIT_FAILURE);
	msleep(100);
	kill(parent, SIGUSR2);
	kill(parent, SIGUSR1);
	msleep(100);
	kill(parent, SIGTERM);
	_exit(EXIT_SUCCESS);
}

int main(void)
{
	struct signalfd_siginfo si;
	struct pollfd pfd[2];
	sigset_t mask;
	char buf[64];
	int fds[2], sfd, running = 1;
	ssize_t n;
	pid_t pid;

	setvbuf(stdout, NULL, _IOLBF, 0);

	/* Step 1: block the signals, so they wait for us instead of acting. */
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	sigaddset(&mask, SIGTERM);
	if (sigprocmask(SIG_BLOCK, &mask, NULL))
		die("sigprocmask");

	/* Step 2: get an fd that becomes readable when one is pending. */
	sfd = signalfd(-1, &mask, SFD_CLOEXEC);
	if (sfd < 0)
		die("signalfd");

	if (pipe(fds))
		die("pipe");
	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0) {
		close(fds[0]);
		child(fds[1]);
	}
	close(fds[1]);

	pfd[0] = (struct pollfd){ .fd = fds[0], .events = POLLIN };
	pfd[1] = (struct pollfd){ .fd = sfd, .events = POLLIN };

	/* Step 3: one loop handles data and signals alike. */
	while (running) {
		if (poll(pfd, 2, -1) < 0)
			die("poll");

		if (pfd[0].revents & POLLIN) {
			n = read(fds[0], buf, sizeof(buf));
			if (n > 0)
				printf("  pipe      : \"%s\"\n", buf);
		}
		if (pfd[0].revents & POLLHUP)
			pfd[0].fd = -1;		/* writer closed: stop polling it */

		if (pfd[1].revents & POLLIN) {
			n = read(sfd, &si, sizeof(si));
			if (n != sizeof(si))
				die("read signalfd");
			printf("  signalfd  : %-24s from pid %u\n",
			       strsignal((int)si.ssi_signo), si.ssi_pid);
			if (si.ssi_signo == SIGTERM) {
				puts("  SIGTERM -> shutting down cleanly");
				running = 0;
			}
		}
	}

	waitpid(pid, NULL, 0);
	close(sfd);
	close(fds[0]);
	return EXIT_SUCCESS;
}
