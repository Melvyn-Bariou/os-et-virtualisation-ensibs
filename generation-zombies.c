#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("Parent PID = %d\n", getpid());
    fflush(stdout);

    while (1) {
        if (fork() == 0) {
            /* l'enfant se termine immédiatement */
            _exit(0);
        }
        /* le process parent n'appelle jamais wait(), d'où le zombie */
        sleep(5);
    }
}

