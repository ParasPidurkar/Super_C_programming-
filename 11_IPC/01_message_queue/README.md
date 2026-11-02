# Message queues

A message queue is a list of messages kept **inside the kernel**. One process puts a message in and another takes it out. Neither process needs to know the other's PID, and they don't even have to run at the same time.

```
 process A                      kernel                       process B
┌──────────┐  mq_send()   ┌──────────────────────┐  mq_receive() ┌──────────┐
│ producer │ ───────────► │ [msg][msg][msg][   ] │ ────────────► │ consumer │
└──────────┘              └──────────────────────┘               └──────────┘
                            /dev/mqueue/learn_mq_chat
```

## Contents

1. [Why message queues?](#why-message-queues)
2. [POSIX vs System V](#posix-vs-system-v)
3. [Build and run](#build-and-run)
4. [The examples, one by one](#the-examples-one-by-one)
5. [POSIX API reference](#posix-api-reference)
6. [System V API reference](#system-v-api-reference)
7. [Limits](#limits)
8. [Inspecting queues from the shell](#inspecting-queues-from-the-shell)
9. [Common pitfalls](#common-pitfalls)
10. [Exercises](#exercises)

---

## Why message queues?

Linux has several IPC mechanisms. They differ in what they move and what they promise:

| Mechanism        | Unit of data          | Unrelated processes? | Outlives the processes? | Kernel copies data? |
|------------------|-----------------------|----------------------|-------------------------|---------------------|
| Pipe             | byte stream           | no (parent/child)    | no                      | yes                 |
| FIFO (named pipe)| byte stream           | yes (by path)        | the name does, the data doesn't | yes         |
| **Message queue**| **whole messages**    | **yes (by name/key)**| **yes, until removed**  | **yes**             |
| Shared memory    | raw memory            | yes                  | yes                     | no (fastest)        |
| Unix socket      | stream or datagrams   | yes                  | no                      | yes                 |

What message queues give you:

- **Message boundaries.** If you send 10 bytes and then 20 bytes, the receiver gets one 10-byte message and then one 20-byte message. With a pipe, the reader can see 30 bytes run together (or 7 + 23) and has to split them itself.
- **Priorities (POSIX) or types (System V).** The receiver doesn't have to take messages in strict FIFO order.
- **Kernel persistence.** The queue and its messages stay until someone removes it, even when no process has it open (see `mq_producer`/`mq_consumer`).
- **Built-in flow control.** When the queue is full, the sender blocks. When it's empty, the receiver blocks.

The trade-off: every message is copied twice (sender → kernel → receiver), and messages have a fixed maximum size. For large data, use shared memory. A common pattern sends small "work is ready" messages through a queue and keeps the data itself in shared memory.

---

## POSIX vs System V

Linux has two separate message queue APIs. This folder covers both.

|                        | POSIX (`<mqueue.h>`)                       | System V (`<sys/msg.h>`)                      |
|------------------------|--------------------------------------------|-----------------------------------------------|
| Identified by          | a name: `"/myqueue"`                       | an integer key, usually from `ftok()`         |
| Handle                 | `mqd_t` (a file descriptor on Linux)       | an `int` queue id                             |
| Ordering               | by **priority**, FIFO within a priority    | by **type**, chosen by the receiver           |
| Async notification     | yes, `mq_notify()`                         | no                                            |
| Works with `poll`/`select`/`epoll` | yes on Linux                   | no                                            |
| Timeouts               | `mq_timedsend` / `mq_timedreceive`         | no, only `IPC_NOWAIT`                         |
| Shell tools            | `ls`/`cat`/`rm` in `/dev/mqueue`           | `ipcs -q`, `ipcrm -q <id>`                    |
| Removal                | `mq_unlink(name)`                          | `msgctl(id, IPC_RMID, NULL)`                  |
| Link with              | `-lrt` (built into glibc 2.34+)            | nothing extra                                 |

**Which one should you use?** Prefer POSIX for new code: the API is cleaner, it has timeouts and notification, and it works with `epoll`. You'll still find System V in older code, and its "receive by type" feature is sometimes handy.

---

## Build and run

You need Linux, a C11 compiler and `make`.

```sh
cd 11_IPC/01_message_queue

make            # build everything
make run        # run examples 01-05 one after another
make check      # run 01-05 under AddressSanitizer + UBSan
make unlink     # delete leftover /learn_mq_* queues after a crash or Ctrl-C
make clean      # delete the binaries
```

To run one example on its own: `make 02_posix_priority && ./02_posix_priority`.

The compiler flags are strict (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`), so any warning fails the build.

### Files

| File                   | Teaches                                                          |
|------------------------|------------------------------------------------------------------|
| `common.h`             | `die()`, `print_attr()`, `msleep()`, shared queue names          |
| `01_posix_basic.c`     | full lifecycle, `fork`, blocking when the queue is full          |
| `02_posix_priority.c`  | priority ordering                                                |
| `03_posix_nonblock.c`  | `O_NONBLOCK`, `EAGAIN`, `EMSGSIZE`, `mq_timedreceive`            |
| `04_posix_notify.c`    | `mq_notify()`: being signalled when a message arrives            |
| `05_sysv_msgqueue.c`   | System V: `msgget`/`msgsnd`/`msgrcv`/`msgctl`, receive by type   |
| `mq_producer.c`        | interactive sender (two-terminal demo)                           |
| `mq_consumer.c`        | interactive receiver (two-terminal demo)                         |

Work through them in order: each one builds on the one before.

---

## The examples, one by one

### 01 — the basic lifecycle

```
mq_open(O_CREAT) → fork() → mq_send() / mq_receive() → mq_close() → mq_unlink()
```

The parent creates a queue with room for **4 messages of up to 64 bytes**, then forks. The child receives one message every 100 ms. The parent sends 6 messages without pausing, followed by `"quit"`.

```
  [created] maxmsg=4 msgsize=64 curmsgs=0 flags=0
  parent -> "message 1"
  parent -> "message 2"
  parent -> "message 3"
  parent -> "message 4"
  parent -> "message 5"
  child  <- "message 1" (10 bytes)
  child  <- "message 2" (10 bytes)
  parent -> "message 6"
  ...
  child  <- "quit" (5 bytes)
  queue closed and unlinked
```

What to notice:

- **The parent stalls.** The queue holds 4 messages, so `mq_send()` blocks until the child has taken one out. Nothing extra was written for this flow control.
- **"message 5" is printed before "child <- message 1".** The child had already taken message 1 off the queue (freeing a slot) but hadn't printed it yet. Output from two processes only shows roughly when things happened.
- **10 bytes for "message 1".** The program sends `strlen + 1` bytes so the terminating NUL goes too. The kernel doesn't know about strings; a message is just bytes.
- **`setvbuf(stdout, NULL, _IOLBF, 0)`.** When stdout is a pipe (e.g. under `make run`), it is fully buffered. Each process would then print everything at exit, and the interleaving would disappear.
- **`mq_close` vs `mq_unlink`.** `mq_close` drops *your descriptor*. `mq_unlink` removes the *name*, and the queue is destroyed once every descriptor is closed. They work like `close()` and `unlink()` on a file.

### 02 — priorities

Six messages are sent in mixed order with priorities 0, 1, 5 and 10. They come out highest priority first, and FIFO within the same priority:

```
  send order:              receive order:
    prio  1  low A           prio 10  high A
    prio 10  high A          prio 10  high B
    prio  5  mid             prio  5  mid
    prio  1  low B           prio  1  low A
    prio 10  high B          prio  1  low B
    prio  0  zero            prio  0  zero
```

On Linux priorities go from 0 to 32767 (`sysconf(_SC_MQ_PRIO_MAX) - 1`). POSIX only guarantees 0 to 31, so portable code should stay in that range. A low-priority message can **starve** if high-priority messages keep arriving.

### 03 — non-blocking mode and the error cases

| Step | Situation                                 | Result                     |
|------|-------------------------------------------|----------------------------|
| 1    | `O_NONBLOCK`, queue full                  | `mq_send` → `EAGAIN`       |
| 2    | message bigger than `mq_msgsize`          | `mq_send` → `EMSGSIZE`     |
| 3    | receive buffer smaller than `mq_msgsize`  | `mq_receive` → `EMSGSIZE`  |
| 4    | `O_NONBLOCK`, queue empty                 | `mq_receive` → `EAGAIN`    |
| 5    | blocking, nothing arrives before deadline | `mq_timedreceive` → `ETIMEDOUT` |

Step 3 catches almost everyone. The waiting message is only 6 bytes, yet an 8-byte buffer is rejected. **The receive buffer must be at least `mq_msgsize`**, however small the actual message is. Size it with `mq_getattr()` (as `mq_consumer.c` does).

Step 5 shows that the timeout is an **absolute `CLOCK_REALTIME` time**, not a duration. You read the clock and add your timeout to it.

`mq_setattr()` can change **only** `O_NONBLOCK`. `mq_maxmsg` and `mq_msgsize` are fixed when the queue is created.

This file also contains a real C trap, explained in a comment. `f(g(), errno)` might read `errno` *before* calling `g()`, because C leaves the order of argument evaluation unspecified. Always store the result first, then check `errno`.

### 04 — notification instead of blocking

Instead of sitting in `mq_receive()`, a process can ask the kernel to signal it when a message arrives:

```c
struct sigevent sev = { .sigev_notify = SIGEV_SIGNAL, .sigev_signo = SIGUSR1 };
mq_notify(mq, &sev);
```

```
  child  -> "a1"
  child  -> "a2"
  parent: SIGUSR1 (si_code SI_MESGQ, sender pid = child)
  parent <- "a1"
  parent <- "a2"
  child  -> "b1"
  parent: SIGUSR1 (si_code SI_MESGQ, sender pid = child)
  parent <- "b1"
  ...
```

The rules, each of which shapes the code:

1. **It fires only when the queue goes from empty to non-empty.** "a2" didn't cause a second signal because the queue already had "a1" in it. So the handler must **drain everything** (non-blocking reads until `EAGAIN`), not just read one message.
2. **It is one-shot.** After firing, the registration is gone, so the code calls `arm()` again every time.
3. **Re-register *before* draining.** If you drain first and register afterwards, a message that arrives in between finds a non-empty queue. It doesn't fire, and you'll never be told about it.
4. **Only one process can be registered per queue.** Others get `EBUSY`.
5. **A blocked `mq_receive()` wins.** If another process is waiting in `mq_receive()`, it gets the message and no notification is sent.

The example blocks `SIGUSR1` and collects it with `sigwaitinfo()`. That avoids writing a signal handler, which may only call async-signal-safe functions. The other option is `SIGEV_THREAD`, where the kernel (through glibc) runs a function on a new thread.

On Linux an `mqd_t` is a real file descriptor, so you can also put it in `poll()`/`epoll` like a socket. That's usually the best choice in an event loop.

### 05 — System V queues

Each message starts with a `long mtype` (which must be > 0). The **receiver** picks what it wants through `msgtyp`:

| `msgtyp` | Returns                                                   |
|----------|-----------------------------------------------------------|
| `0`      | the oldest message of any type (plain FIFO)               |
| `> 0`    | the oldest message of **exactly** that type               |
| `< 0`    | the oldest message with the **lowest** type ≤ `|msgtyp|`  |

```
2. msgtyp > 0: only type 2
  msgrcv(msgtyp= 2) -> type 2  "job: build"
3. msgtyp < 0: lowest type <= 2 first, until none are left
  msgrcv(msgtyp=-2) -> type 1  "urgent: overheat"
  msgrcv(msgtyp=-2) -> type 1  "urgent: fan"
  msgrcv(msgtyp=-2) -> type 2  "job: test"
  msgrcv(msgtyp=-2) -> ENOMSG (nothing matches)
4. msgtyp = 0: whatever is oldest
  msgrcv(msgtyp= 0) -> type 3  "log: disk ok"
  ...
```

A classic use: one server queue and many clients. Each client sends requests as type 1 and waits for replies with `msgtyp = its own PID`, so every client reads only its own answers.

Things to notice:

- **The size passed to `msgsnd`/`msgrcv` excludes `mtype`.** It counts only the payload. Passing `sizeof(struct message)` is a classic bug that reads past your buffer.
- **Too-small buffer:** `msgrcv` fails with `E2BIG` and the message **stays** in the queue. With `MSG_NOERROR` it is truncated instead, and the cut-off bytes are lost.
- **`IPC_PRIVATE`** always makes a new queue, which is handy for parent/child. Unrelated processes agree on a key instead:
  ```c
  key_t key = ftok("/tmp/myapp", 'A');      /* same path + id → same key */
  int id = msgget(key, IPC_CREAT | 0600);
  ```
- **`msgctl(id, IPC_RMID, NULL)`** removes the queue immediately, even if other processes are using it. Without it, the queue stays until reboot. Check with `ipcs -q`.

### mq_producer + mq_consumer — two terminals

This is where "lives in the kernel" becomes concrete.

**Terminal 1:**
```sh
./mq_producer
hello
second
!fire          # leading '!' = priority 9
^D             # Ctrl-D: producer exits
```

The producer has exited, but the messages are still there:

```sh
ls /dev/mqueue
# learn_mq_chat
cat /dev/mqueue/learn_mq_chat
# QSIZE:18   NOTIFY:0   SIGNO:0   NOTIFY_PID:0
```

`QSIZE` is the total number of bytes waiting: "hello\0" + "second\0" + "fire\0" = 6 + 7 + 5 = 18.

**Terminal 2:**
```sh
./mq_consumer
# Reading from /learn_mq_chat (3 already waiting). Waiting for messages...
#   [prio 9] fire
#   [prio 0] hello
#   [prio 0] second
```

The urgent message comes out first even though it was sent last. Now run `./mq_producer` again in terminal 1 and type lines; they appear in terminal 2 straight away. Type `quit` and the consumer exits and calls `mq_unlink()`, so the queue disappears from `/dev/mqueue`.

Try starting the consumer **first**. It blocks in `mq_receive()` until a producer appears. Both programs pass `O_CREAT`, so whichever starts first creates the queue.

---

## POSIX API reference

```c
#include <fcntl.h>
#include <mqueue.h>

mqd_t mq_open(const char *name, int oflag);
mqd_t mq_open(const char *name, int oflag, mode_t mode, struct mq_attr *attr);
int   mq_close(mqd_t mq);
int   mq_unlink(const char *name);

int     mq_send(mqd_t mq, const char *msg, size_t len, unsigned int prio);
ssize_t mq_receive(mqd_t mq, char *buf, size_t len, unsigned int *prio);
int     mq_timedsend(mqd_t, const char *, size_t, unsigned int, const struct timespec *abs);
ssize_t mq_timedreceive(mqd_t, char *, size_t, unsigned int *, const struct timespec *abs);

int mq_getattr(mqd_t mq, struct mq_attr *attr);
int mq_setattr(mqd_t mq, const struct mq_attr *new, struct mq_attr *old);
int mq_notify(mqd_t mq, const struct sigevent *sev);   /* NULL = unregister */

struct mq_attr {
	long mq_flags;    /* 0 or O_NONBLOCK — the only field mq_setattr changes */
	long mq_maxmsg;   /* max messages in the queue  (fixed at creation)       */
	long mq_msgsize;  /* max bytes per message      (fixed at creation)       */
	long mq_curmsgs;  /* messages waiting right now (read-only)               */
};
```

- **Name:** must start with `/` and contain no other `/`. On Linux it shows up as `/dev/mqueue/<name>`.
- **`oflag`:** exactly one of `O_RDONLY`, `O_WRONLY` or `O_RDWR`, optionally ORed with `O_CREAT`, `O_EXCL` and `O_NONBLOCK`.
- **`attr`** is used only when the queue is actually created. Opening an existing queue ignores it. Pass `NULL` to get the system defaults.
- All of these functions return `-1` (or `(mqd_t)-1`) and set `errno` on error.

## System V API reference

```c
#include <sys/ipc.h>
#include <sys/msg.h>

key_t   ftok(const char *path, int proj_id);
int     msgget(key_t key, int msgflg);                 /* IPC_CREAT | IPC_EXCL | 0600 */
int     msgsnd(int id, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int id, void *msgp, size_t msgsz, long msgtyp, int msgflg);
int     msgctl(int id, int cmd, struct msqid_ds *buf); /* IPC_STAT, IPC_SET, IPC_RMID */

struct your_message {
	long mtype;      /* > 0, always first */
	char mtext[N];   /* any layout you like; msgsz counts only this part */
};
```

`msgflg` for send/receive: `IPC_NOWAIT` (don't block; you get `EAGAIN` on send, `ENOMSG` on receive) and `MSG_NOERROR` (truncate long messages).

---

## Limits

POSIX limits on Linux (check yours with `cat`):

| File                                | Default | Meaning                                       |
|-------------------------------------|---------|-----------------------------------------------|
| `/proc/sys/fs/mqueue/msg_max`       | 10      | highest `mq_maxmsg` an unprivileged user can set |
| `/proc/sys/fs/mqueue/msgsize_max`   | 8192    | highest `mq_msgsize` an unprivileged user can set |
| `/proc/sys/fs/mqueue/queues_max`    | 256     | queues on the whole system                    |
| `/proc/sys/fs/mqueue/msg_default`   | 10      | `mq_maxmsg` when `attr` is `NULL`             |
| `/proc/sys/fs/mqueue/msgsize_default` | 8192  | `mq_msgsize` when `attr` is `NULL`            |

Asking for more than `msg_max` gives `EINVAL` from `mq_open`. There is also a per-user byte budget, `ulimit -q` (`RLIMIT_MSGQUEUE`, 819200 bytes by default). Going over it gives `EMFILE`.

For System V, `msg_qbytes` (16384 by default, shown by example 05) caps the total bytes in one queue, and `/proc/sys/kernel/msgmax` caps a single message.

---

## Inspecting queues from the shell

```sh
# POSIX
ls -l /dev/mqueue                 # list queues (owner and mode like files)
cat /dev/mqueue/learn_mq_chat     # QSIZE, notification state
rm /dev/mqueue/learn_mq_chat      # same as mq_unlink()

# System V
ipcs -q                           # list queues: key, id, owner, bytes, messages
ipcs -q -i <id>                   # details for one queue
ipcrm -q <id>                     # same as msgctl(IPC_RMID)
```

If `/dev/mqueue` doesn't exist (in some containers), mount it:
`sudo mkdir -p /dev/mqueue && sudo mount -t mqueue none /dev/mqueue`.

---

## Common pitfalls

| Symptom                                   | Cause                                                                 |
|-------------------------------------------|-----------------------------------------------------------------------|
| `mq_receive`: Message too long            | buffer smaller than `mq_msgsize`; size it from `mq_getattr()`          |
| `mq_open`: Invalid argument               | name doesn't start with `/`, or `mq_maxmsg`/`mq_msgsize` above the limits |
| `mq_open`: File exists                    | `O_EXCL` and a stale queue from an old run (`make unlink`)            |
| `undefined reference to mq_open`          | old glibc: link with `-lrt`                                           |
| Old messages show up in a new run         | the previous run never called `mq_unlink` / `IPC_RMID`                |
| Notification fired once, then never again | `mq_notify` is one-shot; re-register every time, before draining      |
| `mq_timedreceive` returns immediately     | passed a duration (e.g. 1 s) instead of an absolute time              |
| Output printed twice after `fork()`       | stdio buffer copied into the child; `fflush(stdout)` before `fork`    |
| `msgrcv` reads garbage / overflows        | `msgsz` included `sizeof(long)` for `mtype`                           |
| `msgsnd`: Invalid argument                | `mtype` was 0 or negative                                             |

---

## Exercises

1. In `01_posix_basic.c`, change `MAXMSG` to 1, then to 10. How does the parent/child interleaving change? Then try 20. Which error do you get, and why? (Hint: see [Limits](#limits).)
2. In `02_posix_priority.c`, add a message with priority 32768. What does `mq_send` return?
3. Rewrite `04_posix_notify.c` to use `SIGEV_THREAD` instead of a signal. (Link with `-pthread`. The thread function must also re-register and drain.)
4. Replace the notification in `04` with `poll()` on the `mqd_t` and `POLLIN`.
5. Build a request/reply server with System V queues: clients send type 1 messages containing their PID, and the server replies with `mtype = client PID`.
6. Run `mq_consumer` in two terminals and one `mq_producer`. Who gets each message? (Each message goes to exactly one receiver, so the queue acts as a work queue, not a broadcast.)
7. Make `mq_producer` use `mq_timedsend` with a 2-second timeout and print a warning when the consumer is too slow.
