/*
 * ============================================================
 *  FILE: 02_sigaction_function.c
 *  TOPIC: UNIX Signals — Using sigaction() (the CORRECT way)
 * ============================================================
 *
 * WHY sigaction() OVER signal()?
 * --------------------------------
 *  signal() has undefined / non-portable behaviour:
 *   - On some systems the handler is reset to SIG_DFL after each call.
 *   - It does NOT let you control signal masking during handler execution.
 *   - It is NOT safe to use from multi-threaded programs.
 *
 *  sigaction() is the POSIX standard and fixes all these issues:
 *   ✔ Handler stays registered (no auto-reset).
 *   ✔ You can block other signals while handler runs (sa_mask).
 *   ✔ You get extra signal info via siginfo_t (with SA_SIGINFO flag).
 *   ✔ Fully specified restart behaviour for interrupted system calls.
 *
 * struct sigaction — THE KEY STRUCTURE:
 * ----------------------------------------
 *   struct sigaction {
 *       void     (*sa_handler)(int);            // simple handler
 *       void     (*sa_sigaction)(int, siginfo_t*, void*); // extended handler
 *       sigset_t   sa_mask;    // signals to block during handler
 *       int        sa_flags;   // control flags (see below)
 *       void     (*sa_restorer)(void); // obsolete, ignore
 *   };
 *
 * IMPORTANT FLAGS (sa_flags):
 * ----------------------------
 *  SA_RESTART   — Automatically restart interrupted system calls
 *                 (read, write, accept …) instead of returning EINTR.
 *  SA_SIGINFO   — Use the extended sa_sigaction handler instead of
 *                 sa_handler. Provides extra info via siginfo_t.
 *  SA_NOCLDWAIT — Do not create zombie children on SIGCHLD.
 *  SA_ONESHOT   — Reset handler to SIG_DFL after first delivery
 *                 (same broken behaviour as old signal()).
 *
 * siginfo_t — EXTRA SIGNAL INFORMATION (with SA_SIGINFO):
 * ---------------------------------------------------------
 *  si_signo — Signal number
 *  si_pid   — PID of sender (for signals sent by another process)
 *  si_uid   — UID of sender
 *  si_code  — Why signal was generated (SI_USER, SI_KERNEL, …)
 *  si_addr  — Fault address (for SIGSEGV, SIGBUS)
 *  si_value — Real-time signal payload
 *
 * COMPILE:  gcc 02_sigaction_function.c -o sigaction_demo
 * RUN:      ./sigaction_demo
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>   /* memset(), strsignal()          */
#include <signal.h>   /* sigaction(), sigset_t, SIGINT … */
#include <unistd.h>   /* write(), alarm(), pause()       */

/* ------------------------------------------------------------------ */
/* Global flags — volatile sig_atomic_t is the only safe type here    */
/* ------------------------------------------------------------------ */
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t sigusr1_count = 0;

/* ================================================================== */
/*  SIMPLE HANDLER — registered via sa_handler                        */
/*                                                                     */
/*  Receives only the signal number, just like signal() handlers.     */
/*  Used when you do NOT set SA_SIGINFO in sa_flags.                  */
/* ================================================================== */
void simple_sigint_handler(int signum)
{
    (void)signum;
    const char msg[] = "\n[Handler] SIGINT caught — requesting shutdown.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;
    /* Handler stays registered automatically — no re-registration needed */
}

/* ================================================================== */
/*  EXTENDED HANDLER — registered via sa_sigaction (SA_SIGINFO flag)  */
/*                                                                     */
/*  Parameters:                                                        */
/*   signum  — signal number                                           */
/*   info    — pointer to siginfo_t with rich signal details           */
/*   context — pointer to ucontext_t (CPU state — rarely needed)      */
/* ================================================================== */
void extended_sigusr1_handler(int signum, siginfo_t *info, void *context)
{
    (void)signum;  /* signal number is implicit — we know it's SIGUSR1 */
    (void)context; /* suppress unused-parameter warning */

    char buf[128];

    /*
     * info->si_code tells us HOW the signal was generated:
     *   SI_USER   — kill() or raise() by a user process
     *   SI_KERNEL — sent by the kernel
     *   SI_QUEUE  — sent via sigqueue() (with a payload value)
     */
    int n = snprintf(buf, sizeof(buf),
        "[Handler] SIGUSR1 #%d | sender PID=%d | si_code=%d\n",
        (int)++sigusr1_count,
        (int)info->si_pid,
        (int)info->si_code);

    write(STDOUT_FILENO, buf, n);
}

/* ================================================================== */
/*  ALARM HANDLER — simple, for SIGALRM                               */
/* ================================================================== */
void sigalrm_handler(int signum)
{
    (void)signum;
    const char msg[] = "[Handler] SIGALRM — timer expired!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

/* ------------------------------------------------------------------ */
/*  Helper: register a SIMPLE handler (sa_handler) via sigaction()    */
/*  Returns 0 on success, -1 on error.                                */
/* ------------------------------------------------------------------ */
int register_simple_handler(int signum, void (*handler)(int), int flags)
{
    struct sigaction sa;

    /*
     * ALWAYS zero-initialise the struct first.
     * Uninitialised padding bytes can cause subtle bugs.
     */
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handler;   /* set the simple handler */
    sa.sa_flags   = flags;

    /*
     * sa_mask: signals to BLOCK while handler is running.
     * sigemptyset() clears all signals from the mask (nothing blocked).
     * sigemptyset() is the safe, POSIX way to initialise a sigset_t.
     */
    sigemptyset(&sa.sa_mask);

    /*
     * sigaction(signum, &new_action, &old_action)
     *   signum      — which signal to configure
     *   new_action  — new settings
     *   old_action  — where to save the previous settings (NULL = ignore)
     */
    return sigaction(signum, &sa, NULL);
}

/* ------------------------------------------------------------------ */
/*  Helper: register an EXTENDED handler (sa_sigaction, SA_SIGINFO)   */
/* ------------------------------------------------------------------ */
int register_extended_handler(int signum,
                               void (*handler)(int, siginfo_t*, void*),
                               int flags)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = handler;        /* extended handler pointer */
    sa.sa_flags     = flags | SA_SIGINFO; /* MUST include SA_SIGINFO */
    sigemptyset(&sa.sa_mask);

    /* Also block SIGTERM while SIGUSR1 handler runs (example of sa_mask) */
    sigaddset(&sa.sa_mask, SIGTERM);

    return sigaction(signum, &sa, NULL);
}

/* ------------------------------------------------------------------ */
int main(void)
{
    printf("============================================\n");
    printf("  SIGNALS DEMO — sigaction() function\n");
    printf("  PID: %d\n", getpid());
    printf("============================================\n\n");

    /* ============================================================
     * 1. REGISTER SIGINT — simple handler, restart syscalls
     * ============================================================ */
    if (register_simple_handler(SIGINT, simple_sigint_handler,
                                 SA_RESTART) < 0) {
        perror("sigaction(SIGINT)");
        exit(EXIT_FAILURE);
    }
    printf("[Setup] SIGINT  — simple handler registered (SA_RESTART).\n");

    /* ============================================================
     * 2. REGISTER SIGALRM — simple handler
     * ============================================================ */
    if (register_simple_handler(SIGALRM, sigalrm_handler, 0) < 0) {
        perror("sigaction(SIGALRM)");
        exit(EXIT_FAILURE);
    }
    printf("[Setup] SIGALRM — simple handler registered.\n");

    /* ============================================================
     * 3. REGISTER SIGUSR1 — EXTENDED handler with SA_SIGINFO
     *    Gives us the sender's PID, UID, and si_code
     * ============================================================ */
    if (register_extended_handler(SIGUSR1, extended_sigusr1_handler,
                                   SA_RESTART) < 0) {
        perror("sigaction(SIGUSR1)");
        exit(EXIT_FAILURE);
    }
    printf("[Setup] SIGUSR1 — extended SA_SIGINFO handler registered.\n\n");

    /* ============================================================
     * 4. IGNORE SIGTERM temporarily (demonstrate SIG_IGN)
     * ============================================================ */
    struct sigaction ignore_sa;
    memset(&ignore_sa, 0, sizeof(ignore_sa));
    ignore_sa.sa_handler = SIG_IGN;   /* ignore this signal */
    sigemptyset(&ignore_sa.sa_mask);

    if (sigaction(SIGTERM, &ignore_sa, NULL) < 0) {
        perror("sigaction(SIGTERM)");
    }
    printf("[Setup] SIGTERM — temporarily IGNORED.\n\n");

    /* ============================================================
     * 5. DEMONSTRATE raise() — send SIGUSR1 to ourselves
     * ============================================================
     *  raise(sig)  is equivalent to  kill(getpid(), sig)
     *  It sends a signal to the calling process.
     *  The extended handler will print the sender PID (our own PID).
     * ============================================================ */
    printf("[Main] Calling raise(SIGUSR1) three times...\n");
    for (int i = 0; i < 3; i++) {
        if (raise(SIGUSR1) != 0) {
            perror("raise");
        }
    }
    printf("[Main] raise() calls complete.\n\n");

    /* ============================================================
     * 6. DEMONSTRATE alarm()
     * ============================================================
     *  alarm(seconds) — asks the OS to deliver SIGALRM to this
     *                   process after 'seconds' real-time seconds.
     *
     *  alarm(0)       — cancels any pending alarm.
     *
     *  Only ONE alarm can be pending at a time. Setting a new alarm
     *  cancels the previous one and returns its remaining seconds.
     *
     *  Practical use-case: set a timeout before a blocking call.
     *
     *     alarm(5);              set 5-second timeout
     *     n = read(fd, ...);     blocking read
     *     alarm(0);              cancel alarm if read returned in time
     *     if (n < 0 && errno == EINTR) { } -- timeout hit
     *
     * Note: alarm() is NOT suitable for sub-second precision.
     *       Use timer_create() / timerfd for that.
     * ============================================================ */
    printf("[Main] Scheduling SIGALRM in 4 seconds with alarm(4)...\n");
    unsigned int remaining = alarm(4);
    printf("[Main] alarm() returned %u (previous alarm remaining).\n\n", remaining);

    /* ============================================================
     * 7. RESTORE SIGTERM to default before the main loop
     * ============================================================ */
    struct sigaction default_sa;
    memset(&default_sa, 0, sizeof(default_sa));
    default_sa.sa_handler = SIG_DFL;    /* restore OS default */
    sigemptyset(&default_sa.sa_mask);
    sigaction(SIGTERM, &default_sa, NULL);
    printf("[Setup] SIGTERM — restored to SIG_DFL.\n\n");

    /* ============================================================
     * 8. MAIN LOOP
     * ============================================================ */
    printf("[Main] Running. Press Ctrl+C to stop.\n");
    printf("       SIGALRM fires in ~4 seconds.\n\n");

    int tick = 0;
    while (keep_running) {
        sleep(1);
        printf("[Main] tick %d\n", ++tick);
    }

    /* Cancel any pending alarm before exit */
    alarm(0);

    printf("\n[Main] Exited cleanly. SIGUSR1 was raised %d time(s).\n",
           (int)sigusr1_count);
    return 0;
}

/*
 * ============================================================
 * QUICK REFERENCE: signal() vs sigaction()
 * ============================================================
 *
 *  Feature                    signal()       sigaction()
 *  ─────────────────────────────────────────────────────────
 *  POSIX portable             Partially      Yes (POSIX.1)
 *  Handler auto-reset         Yes (some OS)  No (stable)
 *  Block signals in handler   No             Yes (sa_mask)
 *  Extra info (sender PID…)   No             Yes (SA_SIGINFO)
 *  Control SA_RESTART         No             Yes (sa_flags)
 *  Thread-safe                No             Yes (with care)
 *
 * ============================================================
 * raise() vs alarm() vs kill()
 * ============================================================
 *
 *  raise(sig)        — send 'sig' to the calling process itself
 *                      raise(SIGINT) == kill(getpid(), SIGINT)
 *
 *  alarm(seconds)    — schedule SIGALRM after N real-time seconds
 *                      alarm(0) cancels the pending alarm
 *                      returns seconds remaining on prior alarm
 *
 *  kill(pid, sig)    — send 'sig' to process 'pid'
 *                      kill(-1, sig)  → send to ALL processes we own
 *                      kill(-pgid, sig) → send to process group
 *
 * ============================================================
 */
