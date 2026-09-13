# Partie 3 (Linux) : Processus, Inodes, IPC

Compilation : `clang -Wall -Wextra -o <binaire> <source>.c` (ou `gcc`).

## 1. Zombies et signaux
`generation-zombies.c` crée des zombies en boucle. `kill-zombies-signal-autre.c` les moissonne à la réception de `SIGUSR1` (le signal vise le parent, pas les zombies).
```bash
./kill-zombies-signal-autre        # puis, dans un autre terminal :
kill -USR1 <pid>                   # moissonne les zombies
```

## 2. Inodes et blocs
`programme-gestion-inode.c {cas1|cas2|reset}` : cas1 = gros JPEG (données vs espace disque), cas2 = petits fichiers (consommation d'inodes). L'écart théorique/réel du cas2 vient de la table des inodes, allouée au formatage et absente de `st_blocks`.

## 3. Mémoire partagée (IPC)
`corruption-cible` affiche un segment partagé en boucle ; `corruption-attaquant` le déborde (`strcpy`). La cible montre la corruption en temps réel, l'OS reste stable. Nettoyage : `ipcrm -M 1234`.