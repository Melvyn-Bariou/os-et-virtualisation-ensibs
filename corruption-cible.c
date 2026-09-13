/*
 * Cible pour la demonstration de corruption memoire (IPC)
 *
 * Compilation : clang -Wall -Wextra -o cible cible.c
 * Lancement   : ./cible
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>

#define CLE    1234    /* cle partagee : l'attaquant utilisera la meme */
#define TAILLE 64      /* taille du segment, en octets */

int main(void)
{
    int   id;
    char *memoire;

    /* Creer le segment de memoire partagee
       IPC_CREAT : le creer s'il n'existe pas.
       0666      : droits lecture/ecriture pour tous.
    */
    id = shmget(CLE, TAILLE, IPC_CREAT | 0666);
    if (id == -1) {
        perror("shmget");
        return 1;
    }

    /* on obtient un pointeur utilisable comme n'importe quelle zone memoire. */
    memoire = shmat(id, NULL, 0);
    if (memoire == (char *)-1) {
        perror("shmat");
        return 1;
    }

    /* ecriture de la chaine initiale. */
    strcpy(memoire, "Mon message à passer et intact");

    printf("Cible demarree (cle %d, id %d)\n", CLE, id);
    printf("Contenu initial : %s\n\n", memoire);

    /* 4. Boucle d'affichage : on relit le segment chaque seconde. */
    while (1) {
        printf("Contenu : %s\n", memoire);
        sleep(1);
    }

    /* Nettoyer la mémoire avec
       shmdt(memoire); detacher le segment de memoire partagee
       shmctl(id, IPC_RMID, NULL); supprimer le segment 
    */

    return 0;
}