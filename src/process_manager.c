#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include "process_manager.h"
#include "signal_manager.h"
#include "logger.h"

#ifndef MAX_RESTARTS
#define MAX_RESTARTS 3
#endif
#define WORKER_COUNT 2
#ifndef WORKER_PATH
#define WORKER_PATH "./workers/worker"
#endif 

static struct {
    pid_t pid;
    int restarts;
} workers[WORKER_COUNT];
static int restart_enabled = 1;

static int start_worker(int i)
{
    pid_t pid = fork();
    if (pid == -1) {
        log_event("Worker %d: fork failed: %s", i + 1, strerror(errno));
        return -1;
    }
    if (pid == 0) {
        signals_reset_in_child();
        execl(WORKER_PATH, WORKER_PATH, (char *)NULL);
        _exit(127);
    }
    workers[i].pid = pid;
    log_event("Worker %d started (pid %ld, restarts %d)",
              i + 1, (long)pid, workers[i].restarts);
    return 0;
}

int pm_start_all(void)
{
    for (int i = 0; i < WORKER_COUNT; ++i) {
        if (start_worker(i) == -1)
            return -1;
    }
    return 0;
}

void pm_reap(void)
{
    int status;
    for (;;) {
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0)
            break;
        if (pid == -1) {
            if (errno == EINTR)
                continue;
            if (errno != ECHILD)
                log_event("waitpid failed: %s", strerror(errno));
            break;
        }
        for (int i = 0; i < WORKER_COUNT; ++i) {
            if (workers[i].pid != pid)
                continue;
            workers[i].pid = 0;
            if (WIFEXITED(status)) {
                log_event("Worker %d (pid %ld) exited with code %d",
                          i + 1, (long)pid, WEXITSTATUS(status));
                if (WEXITSTATUS(status) == 0)
                    break;
            } else if (WIFSIGNALED(status)) {
                log_event("Worker %d (pid %ld) terminated by signal %d",
                          i + 1, (long)pid, WTERMSIG(status));
            }
            if (!restart_enabled || g_stop_requested)
                break;
            while (workers[i].restarts < MAX_RESTARTS) {
                ++workers[i].restarts;
                log_event("Restarting worker %d (attempt %d/%d)",
                          i + 1, workers[i].restarts, MAX_RESTARTS);
                if (start_worker(i) == 0)
                    break;
            }
            if (workers[i].pid == 0)
                log_event("Worker %d reached restart limit", i + 1);
            break;
        }
    }
}

void pm_periodic_check(void)
{
    pm_reap();
    for (int i = 0; i < WORKER_COUNT; ++i) {
        if (workers[i].pid > 0)
            log_event("Periodic check: worker %d (pid %ld) active", i + 1,
                      (long)workers[i].pid);
        else
            log_event("Periodic check: worker %d inactive", i + 1);
    }
}

int pm_start_timer(int seconds)
{
    struct itimerval timer = {0};
    if (seconds <= 0) {
        errno = EINVAL;
        return -1;
    }
    timer.it_value.tv_sec = seconds;
    timer.it_interval.tv_sec = seconds;
    return setitimer(ITIMER_REAL, &timer, NULL);
}

void pm_stop_timer(void)
{
    struct itimerval timer = {0};
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
        log_event("Could not stop timer: %s", strerror(errno));
}

void pm_disable_restart(void)
{
    restart_enabled = 0;
}

void pm_signal_all(int sig)
{
    for (int i = 0; i < WORKER_COUNT; ++i) {
        if (workers[i].pid > 0 && kill(workers[i].pid, sig) == -1 && errno != ESRCH)
            log_event("kill failed for worker %d: %s", i + 1, strerror(errno));
    }
}

int pm_running_count(void)
{
    int count = 0;
    for (int i = 0; i < WORKER_COUNT; ++i)
        if (workers[i].pid > 0)
            ++count;
    return count;
}
