# Signals

A signal is a tiny asynchronous notification sent to a process: "something happened". The kernel sends them for events like a crash (`SIGSEGV`), a child exiting (`SIGCHLD`) or Ctrl-C (`SIGINT`), and processes send them to each other with `kill()`.

```
 sender                          kernel                          target process
┌─────────┐   kill(pid, SIGTERM)  ┌────────────────────┐  delivers  ┌─────────────────────┐
│ process │ ────────────────────► │ marks SIGTERM      │ ─────────► │ default action, or  │
│ / kernel│                       │ pending for target │            │ your handler runs   │
└─────────┘                       └────────────────────┘            └─────────────────────┘
```

As IPC, signals are the **smallest** mechanism: they carry a number, and at most one extra `int` (`sigqueue`). They're used to *notify* (stop, reload, child exited), never to transfer data. Every Linux program deals with them anyway, because the kernel and the shell use them constantly.

## Contents

1. [Build and run](#build-and-run)
2. [The signals you need to know](#the-signals-you-need-to-know)
3. [The life of a signal](#the-life-of-a-signal)
4. [The examples, one by one](#the-examples-one-by-one)
5. [Writing a safe handler](#writing-a-safe-handler)
6. [Signals across fork, exec and threads](#signals-across-fork-exec-and-threads)
7. [API reference](#api-reference)
8. [Inspecting signals from the shell](#inspecting-signals-from-the-shell)
9. [Common pitfalls](#common-pitfalls)
10. [Exercises](#exercises)

---

## Build and run

You need Linux, a C11 compiler and `make`.

```sh
cd 11_IPC/02_signals

make            # build everything
make run        # run examples 01-05 one after another
make check      # run 01-05 under AddressSanitizer + UBSan
make clean      # delete the binaries
./server        # interactive: poke it with Ctrl-C, Ctrl-Z, kill -HUP ...
```

The compiler flags are strict (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`), so any warning fails the build.

| File                     | Teaches                                                              |
|--------------------------|----------------------------------------------------------------------|
| `common.h`               | `die()`, `say()` (signal-safe print), `install()`, `msleep()`         |
| `01_default_actions.c`   | what each signal does by default; decoding `waitpid()` status        |
| `02_handlers.c`          | `sigaction`, `SA_SIGINFO`, uncatchable signals, `EINTR` vs `SA_RESTART`, `SIG_IGN` |
| `03_masking.c`           | blocking, pending, merging; real-time signals queue and carry data   |
| `04_sigchld.c`           | zombies, reaping with a `SIGCHLD` handler, race-free `sigsuspend()`  |
| `05_signalfd.c`          | Linux `signalfd()`: signals as readable data in a `poll()` loop      |
| `server.c`               | interactive daemon: graceful shutdown, reload on `SIGHUP`, stop/continue |

Work through them in order: each one builds on the one before.

---

## The signals you need to know

Numbers are for x86 and ARM Linux; a few differ on other architectures, so always use the names. `kill -l` lists them all.

| Signal    | No. | Default   | Sent when / by                                       | Catchable |
|-----------|-----|-----------|------------------------------------------------------|-----------|
| `SIGHUP`  | 1   | terminate | terminal closed; by convention "reload your config"  | yes |
| `SIGINT`  | 2   | terminate | **Ctrl-C**                                           | yes |
| `SIGQUIT` | 3   | core      | **Ctrl-\\**                                          | yes |
| `SIGILL`  | 4   | core      | illegal CPU instruction                              | yes |
| `SIGABRT` | 6   | core      | `abort()`, failed `assert()`                         | yes |
| `SIGBUS`  | 7   | core      | bad memory access (e.g. past the end of an `mmap`ed file) | yes |
| `SIGFPE`  | 8   | core      | integer divide by zero                               | yes |
| `SIGKILL` | 9   | terminate | `kill -9`, the OOM killer                            | **no** |
| `SIGUSR1` | 10  | terminate | free for your program to use                         | yes |
| `SIGSEGV` | 11  | core      | invalid memory access (NULL dereference ...)          | yes |
| `SIGUSR2` | 12  | terminate | free for your program to use                         | yes |
| `SIGPIPE` | 13  | terminate | writing to a pipe/socket nobody reads                | yes |
| `SIGALRM` | 14  | terminate | `alarm()` / `setitimer()` timer expired              | yes |
| `SIGTERM` | 15  | terminate | `kill <pid>`, systemd stopping a service             | yes |
| `SIGCHLD` | 17  | ignore    | a child exited, stopped or continued                 | yes |
| `SIGCONT` | 18  | continue  | `fg`, `bg`, `kill -CONT`                             | yes (but it always resumes) |
| `SIGSTOP` | 19  | stop      | `kill -STOP`                                         | **no** |
| `SIGTSTP` | 20  | stop      | **Ctrl-Z**                                           | yes |
| `SIGTTIN` / `SIGTTOU` | 21/22 | stop | background job reads from / writes to the terminal | yes |
| `SIGWINCH`| 28  | ignore    | terminal window resized                              | yes |
| `SIGRTMIN`..`SIGRTMAX` | 34..64 | terminate | real-time signals, free for your use   | yes |

"Core" means terminate **and** write a core dump (a snapshot of memory for debuggers) if `ulimit -c` allows it.

**SIGTERM vs SIGKILL.** `SIGTERM` politely asks the program to exit, and it can clean up first. `SIGKILL` removes it immediately: no handler, no cleanup, no flushing of buffers. systemd and Docker send `SIGTERM`, wait (about 10 s by default), then send `SIGKILL`. Use `kill -9` only as a last resort.

---

## The life of a signal

```
 generated ──► pending ──(not blocked)──► delivered ──► action
                  │                                       ├─ default (terminate / core / stop / ignore)
                  └─(blocked)─► stays pending              ├─ ignored (SIG_IGN)
                                until unblocked            └─ your handler runs
```

1. **Generated.** By the kernel (fault, timer, child exit, terminal key) or by `kill()`, `raise()` or `sigqueue()`.
2. **Pending.** The kernel sets a bit for that signal in the target. **For standard signals it's just one bit**: sending `SIGUSR1` five times before it's delivered still leaves one pending `SIGUSR1`. Real-time signals are queued, one entry per send.
3. **Blocked or not.** Each thread has a *signal mask*. A blocked signal stays pending, neither lost nor delivered, until it's unblocked.
4. **Delivered.** On the way back from the kernel to user space, the process does the signal's *disposition*: the default action, ignore, or your handler.

A handler runs **in the middle of whatever your code was doing**, at any instruction. That is where the rules in [Writing a safe handler](#writing-a-safe-handler) come from.

---

## The examples, one by one

### 01 — default actions

Each signal is sent to a fresh child, and the parent decodes `waitpid()`'s status with `WIFEXITED`, `WIFSIGNALED`, `WTERMSIG`, `WIFSTOPPED` and `WIFCONTINUED`. That's exactly how your shell decides to print "Killed".

```
1. signals whose default action ends the process
  SIGTERM  -> killed by signal 15 (Terminated)
  SIGINT   -> killed by signal 2 (Interrupt)
  SIGKILL  -> killed by signal 9 (Killed)
  SIGUSR1  -> killed by signal 10 (User defined signal 1)
  SIGQUIT  -> killed by signal 3 (Quit)
2. signals ignored by default
  SIGCHLD  -> ignored, child still running
  SIGWINCH -> ignored, child still running
3. stop and continue (what Ctrl-Z and `fg` do)
  SIGSTOP  -> stopped by signal 19 (Stopped (signal))
  SIGCONT  -> continued
  SIGTERM  -> killed by signal 15 (Terminated)
4. the exit status a shell shows: 128 + signal number
  SIGINT -> $? = 130, SIGKILL -> $? = 137
```

Two real-world details are built in:

- **Ignored dispositions are inherited.** A non-interactive shell (a script, `sh -c`, `make`) starts background jobs (`cmd &`) with `SIGINT` and `SIGQUIT` ignored, and children inherit that across `fork()` *and* `exec()`. The demo resets each child to `SIG_DFL` so it shows the real defaults.
- **Dying with a core dump can be slow.** On Ubuntu, `/proc/sys/kernel/core_pattern` hands core dumps to the `apport` crash reporter, and the process isn't gone until apport finishes. The demo waits with a blocking `waitpid()` rather than assuming 50 ms is enough. It also sets `RLIMIT_CORE` to 0 in the child, so no core files end up in this folder.

### 02 — handlers with `sigaction()`

**Part 1: who sent it?** With `SA_SIGINFO`, the handler gets a `siginfo_t` containing the sender's PID, how the signal was sent, and for `sigqueue()` one `int` of data:

```
  caught User defined signal 1 from pid 2422404 (child), si_code SI_USER (kill), value 0
  caught User defined signal 1 from pid 2422403 (me), si_code SI_QUEUE (sigqueue), value 42
```

**Part 2: `SIGKILL` and `SIGSTOP` can't be caught, blocked or ignored.** `sigaction()` returns `EINVAL`. That guarantee is what lets an administrator always stop a process.

**Part 3: `EINTR` and `SA_RESTART`.** The program is blocked in `read()` on a pipe when a timer's `SIGALRM` arrives:

```
  without SA_RESTART:
    [handler] SIGALRM ran
    read() -> -1, errno EINTR          <- the call was abandoned
  with SA_RESTART:
    [handler] SIGALRM ran
    read() -> 5 bytes "data"           <- the kernel restarted it after the handler
```

Without `SA_RESTART`, every slow system call (`read`, `write`, `accept`, `wait` ...) can fail with `EINTR` whenever *any* handled signal arrives, so robust code retries on `EINTR`. `SA_RESTART` makes most of them restart automatically. Some calls never restart, whatever the flags: `nanosleep`, `poll`, `select`, `epoll_wait` and `sigsuspend` (see `man 7 signal`). Sometimes `EINTR` is exactly what you want. `server.c` leaves `SA_RESTART` off so its `nanosleep()` wakes up the moment a signal arrives.

**Part 4: `SIG_IGN`** makes the signal disappear. `SIG_DFL` restores the default.

### 03 — blocking, pending, and merging

```
1. block SIGUSR1 and SIGRTMIN
2. send each of them 5 times while blocked
  handlers run so far: SIGUSR1=0 SIGRTMIN=0
  pending: SIGUSR1=yes SIGRTMIN=yes
3. unblock: everything pending is delivered right away
  SIGUSR1  handler ran 1 time(s) for 5 sent (merged)
  SIGRTMIN handler ran 5 time(s) for 5 sent, values: 10 20 30 40 50
```

This is the most important property of signals for IPC:

- **Standard signals (1–31) merge.** The kernel keeps one pending bit, so you can't count with them. Five `SIGUSR1`s can arrive as one.
- **Real-time signals (`SIGRTMIN`..`SIGRTMAX`) queue.** Every send is delivered, in order, with its own `sival_int`. The queue length is limited by `ulimit -i` (`RLIMIT_SIGPENDING`).

`SIGRTMIN` is 34 with glibc, not 32. glibc reserves two for its threads library, which is why you always write `SIGRTMIN + n`, never a raw number.

Blocking is also how you protect **critical sections**: block a signal while updating data its handler reads, then unblock. `sa_mask` in `struct sigaction` does the same for the duration of a handler.

### 04 — `SIGCHLD`, zombies and `sigsuspend()`

```
1. a zombie: the child exited, nobody waited for it
  child 2422420 state: Z   (Z = zombie, shown as <defunct> by ps)
  after waitpid: state -, exit code 7
2. 5 children exit at almost the same moment
  handler ran 1 time(s) but reaped all 5 children:
    pid 2422675 exited with code 10
    ...
```

- **A zombie** is a child that has exited but whose parent hasn't called `wait()`. Its memory is freed, but its PID and exit status stay in the process table. A long-running server that never reaps its children slowly fills the PID table. (If the parent dies, `init`/systemd adopts its zombies and reaps them.)
- **One `SIGCHLD` can mean many dead children** because standard signals merge. The handler must loop `while (waitpid(-1, &st, WNOHANG) > 0)`. Calling `wait()` once per signal leaks zombies.
- **The handler saves and restores `errno`.** It calls `waitpid()`, which can change `errno`, and it may have interrupted main code that was just about to check `errno`.
- **Race-free waiting.** The naive version has a bug:

  ```c
  while (!done)      /* <- signal arrives HERE, handler sets done ...  */
      pause();       /* ... and pause() then sleeps forever            */
  ```

  The fix: keep the signal **blocked**, and use `sigsuspend(&mask_without_it)`. It unblocks and sleeps *in one atomic step*, so the signal can't slip in between the check and the sleep.

If you don't care about exit statuses at all, `signal(SIGCHLD, SIG_IGN)` tells Linux to reap children automatically, so no zombies are created.

### 05 — `signalfd()`: signals without handlers (Linux)

Block the signals, then ask the kernel for a file descriptor that becomes readable when one is pending:

```c
sigprocmask(SIG_BLOCK, &mask, NULL);
int sfd = signalfd(-1, &mask, SFD_CLOEXEC);
/* poll()/epoll() on sfd, then read() a struct signalfd_siginfo */
```

```
  signalfd  : User defined signal 1    from pid 2422427
  pipe      : "hello over the pipe"
  signalfd  : User defined signal 1    from pid 2422427
  signalfd  : User defined signal 2    from pid 2422427
  signalfd  : Terminated               from pid 2422427
  SIGTERM -> shutting down cleanly
```

No code runs asynchronously, so none of the handler rules apply: you can call `printf`, `malloc`, anything. The signal is just another event in the same loop as your sockets and pipes. Most modern Linux event loops (systemd, libuv, tokio) work this way.

Note that the child sent `SIGUSR2` *then* `SIGUSR1`, but they were read as 1 then 2. When several standard signals are pending at once, Linux delivers the lowest-numbered first.

### server — a well-behaved long-running program

```sh
./server                       # terminal 1
kill -HUP  <pid>               # terminal 2: reload config
kill -USR1 <pid>               #             print status
kill       <pid>               #             SIGTERM: graceful shutdown
```

```
server running as pid 2422958. ...
  working... tick 1
  SIGHUP: reloaded config (version 2)
  SIGUSR1: status: 1 ticks, config v2
  SIGCONT: resumed after being stopped
  working... tick 2

  Terminated: finishing current work, saving state...
  clean shutdown done
```

Things to try:
- **Ctrl-C once:** clean shutdown. **Ctrl-C twice quickly:** the handler calls `_exit(130)` straight away. This "second Ctrl-C forces it" pattern is common in real tools.
- **Ctrl-Z, then `fg`:** the default stop action, then `SIGCONT`, which the server notices.
- **Ctrl-\\:** `SIGQUIT` isn't handled, so the process dies at once ("Quit (core dumped)").
- **`kill -9 <pid>`:** no message, no cleanup. Compare it with plain `kill`.

The handlers only set flags. The main loop notices them and does the real work: reload, print, shut down. That's the right structure for any program that handles signals.

---

## Writing a safe handler

A handler can interrupt your program **in the middle of any function, including `malloc()` or `printf()`**. If the handler then calls `printf()` itself, it can deadlock on stdio's lock or corrupt its buffer. So:

1. **Call only async-signal-safe functions.** `man 7 signal-safety` has the full list. Safe: `write`, `read`, `_exit`, `kill`, `waitpid`, `sigprocmask`, `open`, `close`. **Not** safe: `printf`, `malloc`, `free`, `exit`, `syslog`, and most of the C library. (`common.h`'s `say()` uses `write()` for this reason.)
2. **Communicate through `volatile sig_atomic_t` variables.** `volatile` stops the compiler from caching the value in a register; `sig_atomic_t` is guaranteed to be read and written in one piece.
3. **Keep it tiny.** Set a flag and return. Do the real work in the main loop.
4. **Save and restore `errno`** if the handler calls anything that may change it.
5. **Use `sigaction()`, not `signal()`.** `signal()`'s behaviour has varied across systems: whether the handler resets after one use, whether calls restart, whether the signal is blocked during the handler. `sigaction()` spells all of it out.
6. **Or avoid handlers entirely** with `signalfd()` (Linux) or a thread that calls `sigwait()`.

---

## Signals across fork, exec and threads

| Event       | Handlers               | Ignored (`SIG_IGN`) | Signal mask   | Pending signals |
|-------------|------------------------|---------------------|---------------|-----------------|
| `fork()`    | inherited              | inherited           | inherited     | **cleared** in the child |
| `exec()`    | **reset to default**   | stay ignored        | kept          | kept            |

Handlers are reset on `exec()` because the handler code no longer exists in the new program. Ignored and blocked signals carry over, which is how `nohup` works: it ignores `SIGHUP`, then `exec`s your command.

**Threads:** each thread has its own mask (`pthread_sigmask()`), but dispositions are shared by the whole process. A signal sent to the *process* goes to any one thread that doesn't block it. The usual multi-threaded design is to block all signals in every thread and have one dedicated thread call `sigwait()` (or use `signalfd()`).

---

## API reference

```c
#include <signal.h>

/* sending */
int kill(pid_t pid, int sig);          /* pid > 0: that process; 0: my process group;
                                          -1: everyone I may signal; sig 0: "does pid exist?" */
int raise(int sig);                    /* to myself */
int sigqueue(pid_t pid, int sig, const union sigval value);   /* + one int/pointer */

/* handling */
int sigaction(int sig, const struct sigaction *act, struct sigaction *old);
struct sigaction {
	void   (*sa_handler)(int);                         /* or SIG_DFL / SIG_IGN */
	void   (*sa_sigaction)(int, siginfo_t *, void *);  /* used with SA_SIGINFO */
	sigset_t sa_mask;    /* extra signals blocked while the handler runs */
	int      sa_flags;   /* SA_SIGINFO, SA_RESTART, SA_NOCLDSTOP, SA_RESETHAND, SA_NODEFER */
};

/* masks */
int sigemptyset(sigset_t *), sigfillset(sigset_t *);
int sigaddset(sigset_t *, int), sigdelset(sigset_t *, int), sigismember(const sigset_t *, int);
int sigprocmask(int how, const sigset_t *set, sigset_t *old);  /* SIG_BLOCK / SIG_UNBLOCK / SIG_SETMASK */
int sigpending(sigset_t *set);

/* waiting */
int pause(void);
int sigsuspend(const sigset_t *mask);              /* atomic unblock + sleep */
int sigwait(const sigset_t *set, int *sig);        /* signals as a return value */
int sigwaitinfo(const sigset_t *set, siginfo_t *info);

/* Linux */
#include <sys/signalfd.h>
int signalfd(int fd, const sigset_t *mask, int flags);
```

Timers that send signals: `alarm(seconds)` and `setitimer()` deliver `SIGALRM`; `timer_create()` can send any signal.

---

## Inspecting signals from the shell

```sh
kill -l                        # list signal names and numbers
kill -TERM <pid>               # same as: kill <pid>
kill -s HUP <pid>
kill -0 <pid> && echo alive    # signal 0 = existence/permission check, sends nothing
pkill -USR1 server             # by name
trap 'echo got INT' INT        # handle signals in a shell script

grep Sig /proc/<pid>/status    # SigPnd/ShdPnd = pending, SigBlk = blocked,
                               # SigIgn = ignored, SigCgt = caught (has a handler)
```

The `Sig*` lines are hex bitmasks: bit *n−1* is signal *n*. For example `SigIgn: 0000000000001000` means bit 12 is set, so signal 13 (`SIGPIPE`) is ignored.

`strace -e trace=signal ./prog` shows every signal a program sends and receives.

---

## Common pitfalls

| Symptom                                         | Cause                                                                 |
|-------------------------------------------------|-----------------------------------------------------------------------|
| Program randomly hangs or crashes in a handler  | handler called `printf`/`malloc`; use only async-signal-safe calls    |
| Flag set by the handler is never seen in a loop | flag not `volatile sig_atomic_t`; the compiler cached it              |
| `read()`/`accept()` fail with "Interrupted system call" | `EINTR`: retry, or install the handler with `SA_RESTART`      |
| Counted 3 signals, 5 were sent                  | standard signals merge; use real-time signals or another IPC          |
| Zombies (`<defunct>`) pile up                   | children never waited for; reap in a `waitpid(WNOHANG)` loop          |
| Program sleeps forever after a signal arrived   | "check flag, then `pause()`" race; use `sigsuspend()`                 |
| Server dies silently when a client disconnects  | `SIGPIPE`; ignore it and handle `EPIPE`, or use `MSG_NOSIGNAL`         |
| A program started from a script ignores Ctrl-C  | non-interactive shells start `cmd &` jobs with `SIGINT` ignored, and it's inherited |
| Child doesn't see the handler after `exec`      | `exec` resets handlers to default (ignored signals stay ignored)      |
| `SIGRTMIN` isn't 32                             | glibc reserves 32 and 33; always use `SIGRTMIN + n`                   |

---

## Exercises

1. In `03_masking.c`, also send `SIGRTMIN + 1` while blocked. In what order are `SIGRTMIN` and `SIGRTMIN + 1` delivered?
2. In `04_sigchld.c`, change the handler to call `waitpid()` just once (no loop). How many zombies are left? Check with `ps -o pid,stat,comm --ppid <pid>` while it waits.
3. Add a `SIGWINCH` handler to `server.c` that prints the new terminal size (`ioctl(STDIN_FILENO, TIOCGWINSZ, ...)` in the main loop, not in the handler). Resize your terminal to test it.
4. Rewrite `server.c` with `signalfd()` and `poll()` instead of handlers.
5. Write a "ping-pong": parent and child alternately send each other `SIGUSR1` 10 times, each waiting with `sigsuspend()`. Why do you have to block `SIGUSR1` before `fork()`?
6. Make `server.c` ignore `SIGPIPE`, then run `./server | head -1`. What happens to `printf` once `head` exits?
7. Compare `sleep 100 & grep SigIgn /proc/$!/status` typed at your prompt with `sh -c 'sleep 100 & grep SigIgn /proc/$!/status'`. Decode both masks. Why do they differ?
