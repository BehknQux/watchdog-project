#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "signal_manager.h"
#include "process_manager.h"
#include "logger.h"

int main(void)
{
    if (logger_init(LOG_FILE) == -1) {
        fprintf(stderr, "could not open log file\n");
        return EXIT_FAILURE;
    }

    if (signals_install() == -1 || signals_block() == -1) {
        logger_close();
        return EXIT_FAILURE;
    }

    log_event("Watchdog started (pid %d)", (int)getpid());
    printf("watchdog pid: %d\n", (int)getpid());

    if (pm_start_all() == -1) {
        log_event("Could not start workers");
        logger_close();
        return EXIT_FAILURE;
    }

    while (!g_stop_requested) {
        signals_wait();
    }

    log_event("SIGINT/SIGTERM received");
    pm_signal_all(SIGTERM);
    logger_close();
    return EXIT_SUCCESS;
}
