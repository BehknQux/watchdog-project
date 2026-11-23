#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

#include <signal.h>

/* flags set by the handlers, the real work is done in the main loop */
extern volatile sig_atomic_t g_stop_requested;   /* SIGINT / SIGTERM */
extern volatile sig_atomic_t g_report_requested; /* SIGUSR1 */
extern volatile sig_atomic_t g_child_exited;     /* SIGCHLD */
extern volatile sig_atomic_t g_timer_fired;      /* SIGALRM */

int signals_install(void);

/* blocks the signals, they are only delivered inside signals_wait() */
int signals_block(void);
void signals_wait(void);

/* must be called in the child after fork, before exec (restores the mask) */
void signals_reset_in_child(void);

#endif
