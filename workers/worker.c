#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* usage: worker [rounds]
 * without an argument the worker runs forever,
 * with an argument it exits normally (code 0) after that many rounds */
int main(int argc, char *argv[])
{
    int rounds = 0;

    if (argc > 1)
        rounds = atoi(argv[1]);

    for (int i = 1; rounds <= 0 || i <= rounds; ++i) {
        printf("Worker %ld is running\n", (long)getpid());
        fflush(stdout);
        sleep(3);
    }

    printf("Worker %ld finished normally\n", (long)getpid());
    return EXIT_SUCCESS;
}
