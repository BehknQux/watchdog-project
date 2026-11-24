#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    for (;;) {
        printf("Worker %ld is running\n", (long)getpid());
        fflush(stdout);
        sleep(3);
    }
    return EXIT_SUCCESS;
}
