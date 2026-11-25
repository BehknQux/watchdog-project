#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#define MAX_RESTARTS 3

int pm_start_all(void);          /* starts all workers with fork + exec */
void pm_reap(void);              /* reaps finished children with waitpid(WNOHANG), restarts if needed */
void pm_periodic_check(void);    /* called when SIGALRM arrives */
int pm_start_timer(int seconds); /* periodic SIGALRM using setitimer */
void pm_stop_timer(void);
void pm_disable_restart(void);   /* no new processes once shutdown has started */
void pm_signal_all(int sig);     /* kill() to every running child */
int pm_running_count(void);

/* used by the status report (SIGUSR1) */
int pm_worker_count(void);
int pm_get_worker(int index, int *pid, int *restarts, const char **state);

#endif
/*
