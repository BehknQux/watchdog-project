#ifndef LOGGER_H
#define LOGGER_H

/* Temporary compatible header. If the team's logger.h arrives, keep its
   declarations; src/logger.c must use those exact function signatures. */
#define LOG_FILE "watchdog.log"

int logger_init(const char *path);
void log_event(const char *fmt, ...);
void log_status_report(void);
void logger_close(void);

#endif
