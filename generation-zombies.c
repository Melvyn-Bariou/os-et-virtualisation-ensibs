#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("Parent PID = %d\n", getpid());
    fflush(stdout);

    while (1) {
        if (fork() == 0) {
            /* ENFANT : se termine immédiatement */
            _exit(0);
        }
        /* PARENT : n'appelle jamais wait(), d'où le zombie */
        sleep(5);
    }
}

