# Super C Programming

A hands-on collection of C (and some C++) examples, from "Hello, world" up to libraries, data structures, inter-process communication and threads. Each folder is one topic, with small runnable programs and its own README.

## Topics

| # | Folder | What you'll learn |
|---|--------|-------------------|
| 01 | [`01_Geting_Started`](01_Geting_Started/) | your first C programs: Hello, world |
| 02 | [`02_comments`](02_comments/) | comment styles, commenting out code with the preprocessor and macros, pitfalls, trigraphs |
| 03 | [`03_datatype`](03_datatype/) | precedence, fixed-width integers, signed ints, integer bases, limits, floating point, format specifiers, string literals |
| 04 | [`04_operators.c`](04_operators.c/) | relational, conditional, bitwise, bit masks, short-circuit, arithmetic, member access, indexing tricks, pointer arithmetic, `alignof` |
| 07 | [`07_static_library`](07_static_library/) | building and linking a static library (`.a`) |
| 08 | [`08_dynamic_library`](08_dynamic_library/) | building and linking a shared library (`.so`) |
| 09 | [`09_queue`](09_queue/) | FIFO queues: an array ring buffer and an intrusive linked list, with tests |
| 10 | [`10_bst`](10_bst/) | an intrusive binary search tree: insert, delete, search, in/pre/post-order traversals, with tests |
| 11 | [`11_IPC`](11_IPC/) | inter-process communication and threads (see below) |
| 12 | [`12_templates`](12_templates/) | C++ function and class templates, specialization, CTAD |
| 13 | [`13_memory_layout`](13_memory_layout/) | how a process's memory is laid out: text, rodata, data, bss, heap, stack |
| — | [`signals`](signals/) | a short intro to UNIX signals with `signal()` and `sigaction()` |

### 11_IPC in detail

| Folder | Topic |
|--------|-------|
| [`11_IPC/01_message_queue`](11_IPC/01_message_queue/) | POSIX (`mq_*`) and System V (`msg*`) message queues |
| [`11_IPC/02_signals`](11_IPC/02_signals/) | default actions, `sigaction`, masks, real-time signals, `SIGCHLD`, `signalfd` |
| [`11_IPC/03_threads`](11_IPC/03_threads/) | POSIX threads: create/join, races, mutexes, atomics, condition variables, semaphores, thread-local storage, signals with threads |
| [`11_IPC/04_std_thread`](11_IPC/04_std_thread/) | the same thread lessons in C++20: `std::thread`/`jthread`, `mutex`, `condition_variable`, `counting_semaphore`, `latch`, `thread_local`, `stop_token` |

The top-level [`signals`](signals/) folder is a quick first look at signals. [`11_IPC/02_signals`](11_IPC/02_signals/) covers them in depth.

## Suggested order

1. **Basics:** 01 → 02 → 03 → 04
2. **Building code:** 07 → 08
3. **Data structures:** 09 → 10
4. **How a program runs:** 13 → `signals`
5. **Systems programming:** 11_IPC (message queues → signals → threads → std::thread)
6. **C++:** 12_templates, then 11_IPC/04_std_thread

## Requirements

- Linux (the IPC, signal and thread examples use POSIX APIs)
- `gcc` or `clang`; `g++` 11+ for the C++20 examples
- `make`

## Build and run

Folders with a `Makefile` (`09_queue`, `10_bst`, everything in `11_IPC`):

```sh
cd 11_IPC/03_threads
make          # build
make run      # run all examples
```

Single-file examples:

```sh
gcc -Wall -Wextra 03_datatype/06_limits.c -o limits && ./limits
g++ -std=c++17 -Wall 12_templates/01_function_template.cpp -o tmpl && ./tmpl
```

Each folder's README has the exact commands for that topic.
