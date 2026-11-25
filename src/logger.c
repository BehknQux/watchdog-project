#define _POSIX_C_SOURCE 200809L

#include "logger.h"
#include "process_manager.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static FILE *log_file = NULL;

int logger_init(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (log_file != NULL) {
        errno = EALREADY;
        return -1;
    }

    log_file = fopen(path, "a");
    if (log_file == NULL)
        return -1;

    return 0;
}

void log_event(const char *fmt, ...)
{
    time_t now;
    struct tm local_time;
    char timestamp[32];
    va_list args;

    if (fmt == NULL)
        return;

    /* The project calls this function from the main flow, never a signal handler. */
    FILE *out = (log_file != NULL) ? log_file : stderr;
    now = time(NULL);

    if (now != (time_t)-1 &&
        localtime_r(&now, &local_time) != NULL &&
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time) != 0) {
        fprintf(out, "[%s] ", timestamp);
    } else {
        fputs("[unknown-time] ", out);
    }

    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fputc('\n', out);
    /* The assignment requires each event to be visible immediately on disk. */
    if (fflush(out) == EOF)
        perror("logger: fflush");
}

void log_status_report(void)
{
    int count = pm_worker_count();

    if (count < 0) {
        log_event("STATUS REPORT: could not obtain worker count");
        return;
    }

    log_event("STATUS REPORT: %d worker(s)", count);

    for (int i = 0; i < count; ++i) {
        int pid = -1;
        int restarts = 0;
        const char *state = NULL;

        if (pm_get_worker(i, &pid, &restarts, &state) != 0) {
            log_event("Worker %d: status unavailable", i + 1);
            continue;
        }

        log_event("Worker %d: pid=%d state=%s restarts=%d",
                  i + 1, pid, state != NULL ? state : "UNKNOWN", restarts);
    }
}

void logger_close(void)
{
    if (log_file != NULL) {
        if (fclose(log_file) == EOF)
            perror("logger: fclose");
        log_file = NULL;
    }
}
