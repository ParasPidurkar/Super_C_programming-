/*
 * ============================================================
 *  FILE: 01_signal_function.c
 *  TOPIC: UNIX Signals — Using the classic signal() function
 * ============================================================
 *
 * WHAT IS A SIGNAL?
 * -----------------
 * A signal is a software interrupt delivered to a process by the OS
 * or by another process. It notifies the process that a specific event
 * has occurred (e.g., divide-by-zero, Ctrl+C, timer expiry, etc.)
 *
 * COMMON SIGNALS:
 * ---------------
 *  SIGINT  (2)  — Interrupt from keyboard (Ctrl+C)
 *  SIGTERM (15) — Termination request (default kill)
 *  SIGSEGV (11) — Segmentation fault (invalid memory access)
 *  SIGFPE  (8)  — Floating-point / arithmetic error
 *  SIGALRM (14) — Timer alarm (sent by alarm())
 *  SIGUSR1 (10) — User-defined signal 1
 *  SIGUSR2 (12) — User-defined signal 2
 *  SIGKILL (9)  — Cannot be caught or ignored — kills immediately
 *
 * HOW signal() WORKS:
 * -------------------
 *  signal(signum, handler);
 *
 *  signum  — The signal number to handle (e.g., SIGINT)
 *  handler — One of:
 *              SIG_DFL  → restore default behaviour
 *              SIG_IGN  → ignore the signal
 *              func_ptr → pointer to your custom handler function
 *
 *  Returns: the previous handler (or SIG_ERR on failure)
 *
 * LIMITATION of signal():
 * -----------------------
 *  signal() is considered non-portable and less safe than sigaction().
 *  After a signal is handled, some systems reset the handler back to
 *  SIG_DFL (the default). You must re-register inside the handler to
 *  keep your custom handler active — this is a classic race condition.
 *  Prefer sigaction() (see 02_sigaction_function.c) in production code.
 *
 * COMPILE:  gcc 01_signal_function.c -o signal_demo
 * RUN:      ./signal_demo
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>   /* signal(), raise(), kill(), SIGINT, SIGALRM … */
#include <unistd.h>   /* alarm(), pause(), sleep()                    */

/* ------------------------------------------------------------------ */
/* GLOBAL FLAG — set by handler, read by main loop                    */
/* Using volatile sig_atomic_t is the ONLY safe type to read/write    */
/* from inside a signal handler.                                       */
/* ------------------------------------------------------------------ */
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t alarm_fired  = 0;

/* ================================================================== */
/*  SIGNAL HANDLER for SIGINT (Ctrl+C)                                */
/*                                                                     */
/*  Rules for signal handlers:                                         */
/*   1. Keep them SHORT — do as little work as possible               */
/*   2. Only call async-signal-safe functions (write(), _exit() …)    */
/*   3. Do NOT call printf() — it is NOT async-signal-safe            */
/*   4. Set a flag and let main() react to it                         */
/* ================================================================== */
void sigint_handler(int signum)
{
    (void)signum;
    /*
     * 'signum' carries the actual signal number that triggered this
     * handler. Useful when one handler is registered for many signals.
     */
    const char msg[] = "\n[Handler] Caught SIGINT — setting stop flag.\n";

    /* write() is async-signal-safe; printf() is NOT */
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    /* Tell main() to stop the loop */
    keep_running = 0;

    /*
     * NOTE: On older UNIX systems, the handler is automatically reset
     * to SIG_DFL after it fires once. Re-register here to be safe:
     *   signal(SIGINT, sigint_handler);
     * sigaction() avoids this problem entirely.
     */
}

/* ================================================================== */
/*  SIGNAL HANDLER for SIGALRM                                        */
/*  Fired by alarm() after the specified number of seconds            */
/* ================================================================== */
void sigalrm_handler(int signum)
{
    (void)signum;
    const char msg[] = "[Handler] SIGALRM fired — timer expired!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    alarm_fired = 1;
}

/* ================================================================== */
/*  SIGNAL HANDLER for SIGUSR1                                        */
/*  Demonstrates raise() — sending a signal to yourself               */
/* ================================================================== */
void sigusr1_handler(int signum)
{
    (void)signum;
    const char msg[] = "[Handler] Caught SIGUSR1 — raised by the process itself!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

/* ------------------------------------------------------------------ */
int main(void)
{
    printf("============================================\n");
    printf("  SIGNALS DEMO — signal() function\n");
    printf("  PID: %d\n", getpid());
    printf("============================================\n\n");

    /* ============================================================
     * 1. REGISTER SIGNAL HANDLERS using signal()
     * ============================================================ */

    /* Register handler for SIGINT (Ctrl+C) */
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal(SIGINT)");
        exit(EXIT_FAILURE);
    }
    printf("[Setup] SIGINT  handler registered.\n");

    /* Register handler for SIGALRM */
    if (signal(SIGALRM, sigalrm_handler) == SIG_ERR) {
        perror("signal(SIGALRM)");
        exit(EXIT_FAILURE);
    }
    printf("[Setup] SIGALRM handler registered.\n");

    /* Register handler for SIGUSR1 */
    if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
        perror("signal(SIGUSR1)");
        exit(EXIT_FAILURE);
    }
    printf("[Setup] SIGUSR1 handler registered.\n\n");

    /* ============================================================
     * 2. DEMONSTRATE raise()
     * ============================================================
     * raise(signum) — Sends a signal to the CALLING PROCESS itself.
     *
     * Prototype: int raise(int sig);
     * Returns  : 0 on success, non-zero on failure.
     *
     * It is equivalent to:  kill(getpid(), signum);
     *
     * Use cases:
     *  - Self-testing signal handlers
     *  - Intentionally triggering an error condition
     *  - Resuming default behaviour after custom handling
     * ============================================================ */
    printf("[Main] Sending SIGUSR1 to self via raise()...\n");
    if (raise(SIGUSR1) != 0) {
        perror("raise(SIGUSR1)");
    }
    printf("[Main] Returned from raise() — execution continues.\n\n");

    /* ============================================================
     * 3. DEMONSTRATE alarm()
     * ============================================================
     * alarm(seconds) — Schedules a SIGALRM to be delivered to the
     *                  process after 'seconds' real-time seconds.
     *
     * Prototype: unsigned int alarm(unsigned int seconds);
     * Returns  : Number of seconds remaining on any previously
     *            scheduled alarm (0 if none was pending).
     *
     * Key points:
     *  - Only ONE alarm can be pending at a time.
     *  - alarm(0) cancels any pending alarm.
     *  - When the alarm fires, SIGALRM is sent to the process.
     *  - Default action for SIGALRM is to terminate the process,
     *    so you usually register a handler first (done above).
     *
     * Common use cases:
     *  - Implementing timeouts for blocking calls (read, recv…)
     *  - Periodic tasks (simple, not recommended for precision)
     * ============================================================ */
    printf("[Main] Setting alarm for 3 seconds via alarm(3)...\n");
    unsigned int prev = alarm(3);
    printf("[Main] alarm() returned %u (previous alarm seconds).\n\n", prev);

    /* ============================================================
     * 4. MAIN LOOP — waits until SIGINT (Ctrl+C) stops it
     * ============================================================ */
    printf("[Main] Running main loop. Press Ctrl+C to stop.\n");
    printf("       (SIGALRM will fire in ~3 seconds)\n\n");

    int seconds = 0;
    while (keep_running) {
        sleep(1);
        seconds++;
        printf("[Main] ... %d second(s) elapsed\n", seconds);

        if (alarm_fired) {
            printf("[Main] Detected alarm_fired flag — alarm was handled!\n\n");
            alarm_fired = 0;   /* reset flag */
        }
    }

    /* ============================================================
     * 5. CLEANUP — cancel any pending alarm before exit
     * ============================================================ */
    alarm(0);   /* cancel pending alarm */
    printf("\n[Main] Loop exited cleanly. Goodbye!\n");

    return 0;
}

/*
 * ============================================================
 * EXPECTED OUTPUT (approximate):
 * ============================================================
 *
 *  [Setup] SIGINT  handler registered.
 *  [Setup] SIGALRM handler registered.
 *  [Setup] SIGUSR1 handler registered.
 *
 *  [Main] Sending SIGUSR1 to self via raise()...
 *  [Handler] Caught SIGUSR1 — raised by the process itself!
 *  [Main] Returned from raise() — execution continues.
 *
 *  [Main] Setting alarm for 3 seconds via alarm(3)...
 *  [Main] alarm() returned 0 (previous alarm seconds).
 *
 *  [Main] Running main loop. Press Ctrl+C to stop.
 *         (SIGALRM will fire in ~3 seconds)
 *
 *  [Main] ... 1 second(s) elapsed
 *  [Main] ... 2 second(s) elapsed
 *  [Main] ... 3 second(s) elapsed
 *  [Handler] SIGALRM fired — timer expired!
 *  [Main] ... 4 second(s) elapsed
 *  [Main] Detected alarm_fired flag — alarm was handled!
 *
 *  ^C
 *  [Handler] Caught SIGINT — setting stop flag.
 *  [Main] Loop exited cleanly. Goodbye!
 *
 * ============================================================
 */
