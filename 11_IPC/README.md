# IPC — inter-process communication

Ways for separate processes to exchange data on Linux. Each topic has its own folder with runnable examples, a Makefile and a detailed README.

| Folder                                     | Topic                                              |
|--------------------------------------------|----------------------------------------------------|
| [`01_message_queue`](01_message_queue/)    | POSIX (`mq_*`) and System V (`msg*`) message queues |
| [`02_signals`](02_signals/)                | default actions, `sigaction`, masks, real-time signals, `SIGCHLD`, `signalfd` |
| [`03_threads`](03_threads/)                | pthreads: create/join, races, mutexes, atomics, condition variables, semaphores, TLS, signals with threads |
| [`04_std_thread`](04_std_thread/)          | the same lessons in C++20: `std::thread`/`jthread`, `mutex`, `condition_variable`, `counting_semaphore`, `latch`, `thread_local`, `stop_token` |

Quick start:

```sh
cd 01_message_queue
make run
```
