#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

#include <signal.h>

/* handler'larin set ettigi bayraklar, asil is main dongusunde yapiliyor */
extern volatile sig_atomic_t g_stop_requested;   /* SIGINT / SIGTERM */
extern volatile sig_atomic_t g_report_requested; /* SIGUSR1 */
extern volatile sig_atomic_t g_child_exited;     /* SIGCHLD */
extern volatile sig_atomic_t g_timer_fired;      /* SIGALRM */

int signals_install(void);

/* sinyalleri bloklar, sadece signals_wait() icinde gelmelerine izin verir */
int signals_block(void);
void signals_wait(void);

/* fork'tan sonra cocukta exec'ten once cagrilmali (maskeyi geri acar) */
void signals_reset_in_child(void);

#endif
