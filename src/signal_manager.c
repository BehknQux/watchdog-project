#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "signal_manager.h"

volatile sig_atomic_t g_stop_requested = 0;
volatile sig_atomic_t g_report_requested = 0;
volatile sig_atomic_t g_child_exited = 0;
volatile sig_atomic_t g_timer_fired = 0;

static sigset_t old_mask;

/* handlers only set a flag and do nothing else (async-signal-safe) */
static void on_stop(int sig)
{
    (void)sig;
    g_stop_requested = 1;
}

static void on_usr1(int sig)
{
    (void)sig;
    g_report_requested = 1;
}

static void on_chld(int sig)
{
    (void)sig;
    g_child_exited = 1;
}

static void on_alrm(int sig)
{
    (void)sig;
    g_timer_fired = 1;
}

static int set_handler(int sig, void (*handler)(int), int flags)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = flags;

    if (sigaction(sig, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }
    return 0;
}

int signals_install(void)
{
    if (set_handler(SIGINT, on_stop, SA_RESTART) == -1)
        return -1;
    if (set_handler(SIGTERM, on_stop, SA_RESTART) == -1)
        return -1;
    if (set_handler(SIGUSR1, on_usr1, SA_RESTART) == -1)
        return -1;
    /* no SIGCHLD for stopped children */
    if (set_handler(SIGCHLD, on_chld, SA_RESTART | SA_NOCLDSTOP) == -1)
        return -1;
    if (set_handler(SIGALRM, on_alrm, SA_RESTART) == -1)
        return -1;
    return 0;
}

int signals_block(void)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);

    if (sigprocmask(SIG_BLOCK, &set, &old_mask) == -1) {
        perror("sigprocmask");
        return -1;
    }
    return 0;
}

/* sigsuspend so no signal is lost between checking the flags and waiting */
void signals_wait(void)
{
    sigsuspend(&old_mask);
}

void signals_reset_in_child(void)
{
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGUSR1, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGALRM, SIG_DFL);
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}
