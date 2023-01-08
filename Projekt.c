#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    typedef struct msgbuff
    {
        long mtype;
        char mtext[7];
    } msgbuff;

    FILE *f_konfig = fopen("konfig.txt", "r");
    if (f_konfig == NULL)
    {
        perror("Nie otwarto pliku konfiguracyjnego\n");
        exit(1);
    }

    char *pierwsze_slowo;
    char *drugie_slowo;
    // printf("Szukana\n");
    // printf("%s", argv[1]);
    // printf("\n");
    do
    {
        char tablica_na_zczytana_linie[60];
        if (fgets(tablica_na_zczytana_linie, 60, f_konfig) == NULL)
        {
            perror("Blad czytania z pliku konfiguracyjnego\n");
            exit(1);
        }
        char korektor[] = ": ";
        pierwsze_slowo = strtok(tablica_na_zczytana_linie, korektor);
        // printf("Pierwsze\n");
        // printf("%s", pierwsze_slowo);
        drugie_slowo = strtok(NULL, korektor);
        // printf("\nDrugie\n");
        // printf("%s", drugie_slowo);

    } while (strcmp(pierwsze_slowo, argv[1]) != 0); // rob dopoki te lancuchy sa rozne
    // tutaj mam juz znalezione ID tej kolejki z argv[1]
    int ID_kolejki = atoi(drugie_slowo);
    // konwertuje ze stringa na int drugie slowo i tworze kolejke komunikatow z tym wlasnie ID

    int msgid = msgget(ID_kolejki, IPC_CREAT | 0640);
    printf("JEBANE DZIALA\n");

    //     // szukam w pliku konfig konfiguracji dla procesu o nazwie równej argv[1]

    // if (fork() != 0)
    // {
    //     // robie rzeczy z sekcji PROCES GŁÓWNY w PPW.txt
    // }
    // else
    // {
    //     // robie rzeczy z sekcji PROCES POTOMNY w PPW.txt
    // }
}