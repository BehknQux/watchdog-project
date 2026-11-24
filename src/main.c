#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "signal_manager.h"
#include "process_manager.h"
#include "logger.h"

#define CHECK_INTERVAL 2 /* periodic check interval (seconds) */

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

    if (pm_start_timer(CHECK_INTERVAL) == -1)
        log_event("Could not set up timer, periodic check disabled");

    while (!g_stop_requested) {
        signals_wait();

        if (g_child_exited) {
            g_child_exited = 0;
            pm_reap();
        }
        if (g_timer_fired) {
            g_timer_fired = 0;
            pm_periodic_check();
        }
        if (g_report_requested) {
            g_report_requested = 0;
            log_status_report();
        }

        /* exit if every child is done and nothing is left to restart */
        if (pm_running_count() == 0) {
            log_event("No running children left");
            break;
        }
    }

    if (g_stop_requested)
        log_event("SIGINT/SIGTERM received");

    pm_stop_timer();
    pm_signal_all(SIGTERM);
    logger_close();
    return EXIT_SUCCESS;
}
