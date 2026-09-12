// Script pour manipuler les Inodes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>
#include <dirent.h>

#define DOSSIER "demo_inodes"
#define FICHIER_JPG DOSSIER "/gros_fichier.jpg"
#define DOSSIER_PETITS DOSSIER "/petits"
#define NB_PETITS 20000
#define TAILLE_PETIT 5

static int cas1(void)
{
    // Cas 1 : Création d'un fichier jpg et calcul de l'espace disque utilisé par ce fichier (théorique / réel)
    unsigned char magic[3] = { 0xFF, 0xD8, 0xFF };
    char tampon[4096]; // Un tampon de 4 Ko pour remplir le fichier (etant la taille du bloc ext4)
    int fd; // Descripteur de fichier
    long i;

    // Creer un dossier pour reset plus facilement
    if (mkdir(DOSSIER, 0755) == -1 && errno != EEXIST) {
        perror(DOSSIER);
        return 1;
    }

    // Créer le fichier et écrire les 3 octets magiques
    fd = open(FICHIER_JPG, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    write(fd, magic, 3);

    // Rempli le tampon en mémoire
    for (i = 0; i < 4096; i++)
        tampon[i] = 'X';

    /* Ecrire 1000 fois le tampon de 4 Ko dans le fichier => fichier de 4mo */
    /* Le fichier contient que des 'X' */
    for (i = 0; i < 1000; i++)
        write(fd, tampon, 4096);

    close(fd);

    // Determiner l'espace theorique et réel utilisé par le fichier
    struct stat st; // contenu de l'inode
    struct statvfs fs; // contenu du système de fichiers

    if (stat(FICHIER_JPG, &st) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    if (statvfs(FICHIER_JPG, &fs) == -1) {
        perror("statvfs");
        exit(EXIT_FAILURE);
    }

    long long taille_bloc = fs.f_frsize;
    long long donnees = (long long)st.st_size;
    long long alloue  = (long long)st.st_blocks * 512;
    long long nb_blocs = (donnees + taille_bloc - 1) / taille_bloc;
    long long theorique = nb_blocs * taille_bloc;

    printf("Inode : %lu\n", (unsigned long)st.st_ino);
    printf("Taille de bloc : %lld octets\n", taille_bloc);
    printf("Taille donnees : %lld octets\n", donnees);
    printf("Blocs necessaires : %lld\n", nb_blocs);
    printf("Espace theorique : %lld octets\n", theorique);
    printf("Espace alloue : %lld octets\n", alloue);
    printf("Perte (fragment.) : %lld octets\n", alloue - donnees);

    /*
        Résultats obtenus sur un système de fichiers ext4 avec une taille de bloc de 4 ko :
        Inode : 4467305
        Taille de bloc : 4096 octets
        Taille donnees : 4096003 octets
        Blocs necessaires : 1001
        Espace theorique : 4100096 octets
        Espace alloue : 4100096 octets
        Perte (fragment.) : 4093 octets
    */
}

static int cas2(void)
{
    // Cas 2 : créer un dossier qui va saturer le nombre d'inode avec pleins de petits fichier et d'afficher différentes stats

    
}

/* Supprime tous les fichiers d'un dossier, puis le dossier lui-meme. */
static int vider_dossier(const char *chemin)
{
    DIR *rep;
    struct dirent *entree;
    char cible[512];
    unsigned long n = 0;

    rep = opendir(chemin);
    if (rep == NULL) {
        if (errno == ENOENT)
            return 0;          /* deja absent : rien a faire */
        perror(chemin);
        return -1;
    }

    while ((entree = readdir(rep)) != NULL) {
        /* "." et ".." sont toujours presents, il faut les ignorer */
        if (strcmp(entree->d_name, ".") == 0 || strcmp(entree->d_name, "..") == 0)
            continue;

        snprintf(cible, sizeof cible, "%s/%s", chemin, entree->d_name);

        if (unlink(cible) == -1)
            perror(cible);
        else
            n++;
    }
    closedir(rep);

    printf("  %lu fichier(s) supprime(s) dans %s\n", n, chemin);

    if (rmdir(chemin) == -1 && errno != ENOENT) {
        perror(chemin);
        return -1;
    }
    return 0;
}

static int reset(void)
{
    printf("Nettoyage de la demo...\n");

    vider_dossier(DOSSIER_PETITS);

    if (unlink(FICHIER_JPG) == -1) {
        if (errno != ENOENT)
            perror(FICHIER_JPG);
    } else {
        printf("  %s supprime\n", FICHIER_JPG);
    }

    if (rmdir(DOSSIER) == -1) {
        if (errno != ENOENT)
            perror(DOSSIER);
    } else {
        printf("  %s supprime\n", DOSSIER);
    }

    printf("Termine.\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage : %s {cas1|cas2|reset}\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "cas1") == 0)  return cas1();
    if (strcmp(argv[1], "cas2") == 0)  return cas2();
    if (strcmp(argv[1], "reset") == 0) return reset();

    printf("Commande inconnue : %s\n", argv[1]);
    return 1;
}