/* Logger-only smoke test; fake process manager data is intentionally
   limited to this executable and is never linked into watchdog. */
#define _POSIX_C_SOURCE 200809L
#include "logger.h"
#include "process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pm_worker_count(void)
{
    return 2;
}

int pm_get_worker(int index, int *pid, int *restarts, const char **state)
{
    if (index < 0 || index >= 2 || pid == NULL || restarts == NULL || state == NULL)
        return -1;
    *pid = 1234 + index;
    *restarts = index;
    *state = index == 0 ? "RUNNING" : "STOPPED";
    return 0;
}

int main(void)
{
    char line[512];
    FILE *file;
    int saw_start = 0, saw_status = 0, saw_pid = 0;
    const char *path = "logger_smoke.log";

    if (logger_init(path) != 0) {
        perror("logger_init");
        return EXIT_FAILURE;
    }
    log_event("Worker %d started (pid %d)", 1, 1234);
    log_status_report();
    logger_close();

    file = fopen(path, "r");
    if (file == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != '[') {
            fprintf(stderr, "Missing timestamp: %s", line);
            fclose(file);
            return EXIT_FAILURE;
        }
        if (strstr(line, "Worker 1 started") != NULL) saw_start = 1;
        if (strstr(line, "STATUS REPORT") != NULL) saw_status = 1;
        if (strstr(line, "pid=1234") != NULL) saw_pid = 1;
    }
    fclose(file);
    remove(path);

    if (!saw_start || !saw_status || !saw_pid) {
        fprintf(stderr, "Logger smoke test failed (start=%d status=%d pid=%d)\n",
                saw_start, saw_status, saw_pid);
        return EXIT_FAILURE;
    }
    puts("PASS: logger append, timestamp, event, status report and flush");
    return EXIT_SUCCESS;
}
