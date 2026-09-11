#include <stdio.h>
#include <unistd.h>
#include <signal.h> /* sigaction, SIGUSR1 */
#include <sys/wait.h> /* waitpid, WNOHANG */
#include <string.h> /* memset, strlen */

static void moissonnerProcessEnfants(int sig)
{
    (void)sig;  /* on n'utilise pas le numéro du signal */

    /*On récupère le code de retour de chaque enfant terminé, ce qui libère son entrée dans la table des processus. */
    while (waitpid(-1, NULL, WNOHANG) > 0);

    /*write etant async signal safe */
    const char *msg = "Zombies moissonnes\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof sa); /*mettre les champs de la structure à zero*/
    sa.sa_handler = moissonnerProcessEnfants; /*fonction à appeler */
    sigemptyset(&sa.sa_mask); /*aucun signal bloqué en plus*/
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {  /* erreur */
        perror("sigaction");
        return 1;
    }

    printf("Parent PID = %d\n", getpid());
    printf("Moisson : kill -USR1 %d\n", getpid());
    fflush(stdout);

    while (1) {
        if (fork() == 0) {
            _exit(0);   /* enfant terminé immédiatement */
        }
        sleep(5);       /* parent sans wait(), d'ou le zombie */
    }
}

// Commande pour observer les process zombies : ps aux | grep defunct
// Commande pour envoyer le signal : kill -USR1 pid_du_parent