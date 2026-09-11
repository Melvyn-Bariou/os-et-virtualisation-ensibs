# Notes de TP — Systèmes d'exploitation

## Partie Linux : processus et signaux

---

## 1. La création de processus avec `fork()`

### Origine des processus

Sous Linux, tout processus descend d'un ancêtre commun : **`init`** (ou **`systemd`** sur les
distributions modernes), qui porte le **PID 1**. Il est lancé par le noyau au démarrage, et
tous les autres processus sont créés à partir de lui, de proche en proche.

> À noter : dans les vidéos de Jacob Sorber, on entend parler de `kernel_task`. C'est la
> terminologie **macOS**. L'équivalent Linux est `init` / `systemd`.

La commande `pstree` permet de visualiser cette arborescence.

### Le principe de `fork()`

`fork()` **duplique** le processus appelant. On peut le lire comme un *« clone-moi »* :
le système crée une copie quasi identique du processus, qui reprend l'exécution
**à la même ligne**, juste après l'appel.

L'enfant hérite du parent :

- une copie de l'espace mémoire (via *copy-on-write* : la copie physique n'a lieu qu'à
  la première écriture, ce qui rend `fork()` peu coûteux) ;
- les descripteurs de fichiers ouverts ;
- le répertoire courant, les variables d'environnement, le masque de signaux.

### Distinguer le parent de l'enfant

C'est le point qui déroute au début : **`fork()` retourne deux fois**, une fois dans chaque
processus, avec une valeur différente.

| Valeur retournée | Signification |
|------------------|---------------|
| `0`              | On est dans l'**enfant** |
| `> 0`            | On est dans le **parent** ; la valeur est le PID de l'enfant |
| `-1`             | Échec de la création (ressources épuisées) |

C'est cette différence de valeur de retour qui permet de faire diverger le code
avec un simple `if`.

### Exemple minimal

```c
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }
    else if (pid == 0) {
        printf("Enfant : mon PID est %d, mon parent est %d\n",
               getpid(), getppid());
    }
    else {
        printf("Parent : mon PID est %d, mon enfant est %d\n",
               getpid(), pid);
    }
    return 0;
}
```

Compilation :

```bash
gcc -Wall -Wextra -o fork_demo fork_demo.c
```

> **Erreur corrigée :** dans ma première version j'avais écrit
> `printf("Je suis le parent.\n", getpid());` — l'argument `getpid()` était passé
> sans directive `%d` correspondante dans la chaîne de format. Le compilateur le
> signale avec `-Wall`.

### Remplacer l'image du processus : la famille `exec`

`fork()` duplique, mais l'enfant exécute toujours le même programme. Pour lui faire
exécuter **un autre binaire**, on utilise un appel de la famille `exec` juste après
le fork. Le processus conserve son PID mais son code est intégralement remplacé.

Les variantes se lisent par leurs suffixes :

| Suffixe | Signification |
|---------|---------------|
| `l`     | arguments passés en **l**iste (`execl`, `execlp`) |
| `v`     | arguments passés dans un **v**ecteur / tableau (`execv`, `execvp`) |
| `p`     | recherche le binaire dans le **PATH** |
| `e`     | permet de fournir un **e**nvironnement personnalisé |

C'est le couple `fork()` + `exec()` qui est à la base du fonctionnement d'un shell.

---

## 2. Les processus zombies

### Définition

Un **zombie** (état `Z`, affiché `<defunct>` par `ps`) est un processus qui **s'est terminé**
mais dont le parent **n'a pas encore lu le code de retour**.

Le noyau ne peut pas supprimer complètement son entrée dans la table des processus :
il doit conserver le code de sortie au cas où le parent viendrait le réclamer.

Conséquences pratiques :

- un zombie ne consomme **ni CPU ni mémoire** ;
- il occupe en revanche une **entrée dans la table des processus**, ressource limitée ;
- **on ne peut pas le tuer** — même avec `kill -9`, il est déjà mort.

### Comment s'en débarrasser

Deux mécanismes seulement :

1. Le parent appelle `wait()` ou `waitpid()` : il récupère le code de retour et
   l'entrée est libérée. On parle de **moissonner** (*to reap*) l'enfant.
2. Le parent meurt : les zombies sont alors **adoptés par `init` (PID 1)**,
   qui les nettoie automatiquement.

### Programme générant des zombies

```c
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
```

**Deux points de vigilance :**

- `while (true)` nécessite `#include <stdbool.h>` en C. Sans cet en-tête,
  écrire `while (1)`.
- Dans l'enfant, utiliser **`_exit(0)` et non `return 0` ou `exit(0)`**.
  `exit()` vide les tampons de la stdio hérités du parent, ce qui provoque
  des affichages dupliqués. `_exit()` termine immédiatement sans nettoyage.

### Observer les zombies

```bash
ps -el | grep defunct      # liste les processus en état Z
ps -o pid,ppid,stat,cmd    # la colonne STAT affiche Z
```

---

## 3. Les signaux

### Principe

Un signal est une **notification asynchrone** envoyée à un processus. Il interrompt
le flot normal d'exécution pour déclencher un gestionnaire, puis l'exécution reprend.

### Signaux courants

| Signal | N° | Rôle | Interceptible ? |
|--------|-----|------|-----------------|
| `SIGINT`  | 2  | Interruption clavier (Ctrl+C) | Oui |
| `SIGKILL` | 9  | Terminaison **immédiate et forcée** | **Non** |
| `SIGUSR1` | 10 | Libre, usage applicatif | Oui |
| `SIGTERM` | 15 | Demande **polie** de terminaison (défaut de `kill`) | Oui |
| `SIGUSR2` | 12 | Libre, usage applicatif | Oui |
| `SIGCHLD` | 17 | Envoyé au parent quand un enfant se termine | Oui |

Deux signaux seulement ne peuvent être ni interceptés ni ignorés : **`SIGKILL`** et
**`SIGSTOP`**. C'est ce qui fait de `kill -9` la solution de dernier recours.

> **Erreur corrigée :** j'avais noté que SIGTERM « indique sévèrement » au processus
> de se terminer. C'est l'inverse : SIGTERM est la demande **courtoise**, que le
> programme peut intercepter pour se fermer proprement (sauvegarder, fermer ses
> fichiers). Le signal brutal est SIGKILL, qui ne laisse aucune chance au processus.

`SIGUSR1` et `SIGUSR2` existent précisément pour les usages libres : c'est ce que
l'énoncé du TP appelle un « signal personnalisé ».

### Intercepter Ctrl+C (SIGINT)

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static void gestionnaire(int sig)
{
    (void)sig;
    const char *msg = "\nCtrl+C intercepte, je continue !\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = gestionnaire;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    while (1) {
        printf("Je tourne... (PID %d)\n", getpid());
        sleep(2);
    }
}
```

Pour arrêter ce programme, il faudra un `kill -9` depuis un autre terminal,
puisque Ctrl+C ne fonctionne plus.

### `signal()` ou `sigaction()` ?

`signal()` est l'interface historique, plus courte à écrire, mais son comportement
**varie selon les systèmes** (le gestionnaire est parfois réinitialisé après le
premier déclenchement).

`sigaction()` est l'interface POSIX moderne, au comportement défini et portable.
Elle donne accès à des options supplémentaires via `sa_flags` :

| Flag | Effet |
|------|-------|
| `SA_RESTART` | Les appels système interrompus (comme `sleep`) reprennent automatiquement |
| `SA_NOCLDWAIT` | Les enfants ne deviennent jamais zombies |
| `SA_SIGINFO` | Le gestionnaire reçoit des informations détaillées sur le signal |

**Préférer `sigaction()` systématiquement.**

### Contrainte importante : les fonctions async-signal-safe

Un gestionnaire de signal peut s'exécuter **à n'importe quel moment**, y compris au
milieu d'un `printf()` du programme principal. Si le gestionnaire appelle lui-même
`printf()`, l'état interne de la stdio peut être corrompu.

Seul un sous-ensemble de fonctions est garanti **async-signal-safe**, dont
`write()`, `_exit()`, `waitpid()`, `kill()`. En pratique, dans un gestionnaire :

- utiliser `write()` plutôt que `printf()` ;
- ne partager avec le programme principal que des variables
  déclarées `volatile sig_atomic_t` ;
- sauvegarder et restaurer `errno` si on appelle des fonctions qui le modifient.

### Envoyer un signal

```bash
kill -USR1 1234          # par nom
kill -10 1234            # par numéro
kill -l                  # liste tous les signaux disponibles
```

Depuis un programme : `kill(pid, SIGUSR1);`

---

## 4. Synthèse pour l'exercice du TP

**Énoncé :** générer des zombies à intervalle régulier, et pouvoir les supprimer
via un signal personnalisé (autre que `-9`).

Le raisonnement clé : **on ne peut pas tuer un zombie**, il est déjà mort. Le signal
ne peut donc pas viser les enfants — il doit viser le **parent**, qui réagira en
appelant `waitpid()` pour les moissonner.

Squelette de la logique :

```
main()
 ├── installer le gestionnaire de SIGUSR1 via sigaction()
 ├── afficher son propre PID (sinon on ne sait pas où envoyer le signal)
 └── boucle infinie
      ├── fork()
      │    ├── enfant  → _exit(0) immédiat
      │    └── parent  → n'appelle PAS wait() → le zombie apparaît
      └── sleep(N)

gestionnaire SIGUSR1
 └── while (waitpid(-1, NULL, WNOHANG) > 0) → moissonner tous les zombies
```

Le `WNOHANG` évite de bloquer s'il n'y a rien à récupérer, et le `-1` signifie
« n'importe quel enfant ».

### Commandes de test

```bash
gcc -Wall -Wextra -o zombies zombies.c
./zombies 2

# Dans un second terminal :
ps -el | grep defunct        # les zombies s'accumulent
kill -USR1 <pid_du_parent>
ps -el | grep defunct        # ils ont disparu
```

### Observation avec strace

```bash
strace -f -e trace=clone,wait4,rt_sigaction ./zombies 2
```

On voit les `clone()` s'enchaîner sans aucun `wait4()`, puis une rafale de
`wait4()` au moment de l'envoi du signal.

---

## 5. Expériences complémentaires à mener

- Remplacer `sa_flags = 0` par `SA_RESTART` et observer l'effet sur `sleep()` :
  avec le flag, l'appel reprend automatiquement ; sans, il est interrompu et
  retourne le temps restant.
- Tenter `kill -9 <pid_d_un_zombie>` : il ne se passe rien, ce qui démontre
  qu'un zombie n'est plus un processus vivant.
- Tuer le parent et vérifier avec `ps` que les zombies disparaissent,
  adoptés puis nettoyés par `init`.
- Comparer la sortie de `strace` entre une version C et une version Python
  du même programme.