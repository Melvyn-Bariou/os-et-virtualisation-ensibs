/*
 * Programme de l'ataquant pour corruption de la memoire partagee
 *
 * Compilation : clang -Wall -Wextra -o attaquant attaquant.c
 * Lancement   : ./attaquant [chaine]
 *
 * A lancer pendant que ./cible tourne dans un autre terminal.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>

#define CLE    1234    /* mettre la meme cle que la cible */
#define TAILLE 64      /* taille annoncee par la cible */

int main(int argc, char *argv[])
{
    int   id;
    char *memoire;
    const char *charge;

    /* Charge utile : par defaut, une chaine bien plus longue que les
       64 octets du segment. On peut aussi la passer en argument. */
    if (argc > 1)
        charge = argv[1];
    else
        charge = "La mémoire est corrompue !!! "
                 "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    /* Recuperer le segment existant. Pas de IPC_CREAT car on
       s'attache a celui que la cible a deja cree, pas en creer un. */
    id = shmget(CLE, TAILLE, 0666);
    if (id == -1) {
        perror("shmget");
        fprintf(stderr, "La cible tourne-t-elle ? (segment introuvable)\n");
        return 1;
    }

    /* S'attacher au meme segment */
    memoire = shmat(id, NULL, 0);
    if (memoire == (char *)-1) {
        perror("shmat");
        return 1;
    }

    printf("Attache au segment (cle %d, id %d)\n", CLE, id);
    printf("Contenu avant : %s\n", memoire);
    printf("Taille du segment : %d octets\n", TAILLE);
    printf("Longueur ecrite   : %zu octets\n", strlen(charge) + 1);

    /* Etape du Buffer overflow : strcpy ne verifie aucune limite. On ecrit
       une chaine plus longue que la taille, ce qui deborde au-dela du
       segment et corrompt la memoire adjacente (regarder cela avec la commande objdump) */
    strcpy(memoire, charge);

    printf("Debordement effectue.\n");
    printf("Retourner sur le terminal de la cible : le contenu a change.\n");

    /* Se detacher du segment */
    shmdt(memoire);
    return 0;
}