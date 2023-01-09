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

int zwroc_ID(char *user_name)
{
    FILE *f_konfig = fopen("konfig.txt", "r");
    if (f_konfig == NULL)
    {
        perror("Nie otwarto pliku konfiguracyjnego\n");
        exit(1);
    }

    char *pierwsze_slowo;
    char *drugie_slowo;

    do // szukam w pliku konfig konfiguracji dla procesu o nazwie równej argv[1]
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

    } while (strcmp(pierwsze_slowo, user_name) != 0); // rob dopoki te lancuchy sa rozne

    int ID_kolejki = atoi(drugie_slowo);
    if (ID_kolejki == 0)
    {
        perror("Nie udalo sie odczytac ID kolejki z pliku konfiguracyjnego\n");
        exit(1);
    }

    return ID_kolejki;
    fclose(f_konfig);
}

int main(int argc, char *argv[])
{
    typedef struct msgbuff
    {
        long mtype;
        char mtext[7];
    } msgbuff;

    int ID_kolejki = zwroc_ID(argv[1]);
    int msgid_1 = msgget(ID_kolejki, IPC_CREAT | 0640);
    if (msgid_1 == -1)
    {
        perror("Blad tworzenia pierwszej kolejki\n");
        exit(1);
    }
    printf("msgid %d\n", msgid_1);
    printf("\nJEBANE DZIALA\n");

    switch (fork())
    {
    case 0: // dla potomnego
    {
        while (1)
        {
            // sprawdzam kolejkę dla mojego procesu (np. tutaj odpalony był usr1 więc sprawdzam czy inny proces tu cos nie wpisal)
            msgbuff m;
            int rozmiar_komunikatu = 0;
            rozmiar_komunikatu = msgrcv(msgid_1, &m, (sizeof(msgbuff) - sizeof(long)), 1, 0);
            if (rozmiar_komunikatu == -1)
            {
                perror("Blad odbierania");
                exit(1);
            }
            printf("Odebrano: %s", m.mtext);

            // 2. jak coś jest to wczytuje polecenie i nazwe kolejki pomocniczej - inaczej petla leci od poczatku
            if (rozmiar_komunikatu > 0)
            {
                char terminal[60];
                strcpy(terminal, m.mtext);

                char *proces;
                char *polecenie;
                char *pomocnicza;
                char rozdzielacz[] = " ";
                proces = strtok(terminal, rozdzielacz);
                polecenie = strtok(NULL, rozdzielacz);
                pomocnicza = strtok(NULL, rozdzielacz);

                // 3. wykonuje polecenie
                // 4. wynik polecenia wpisuje do kolejki pomocniczej
            }
        }

        break;
    }
    default: // dla macierzystego
    {
        while (1)
        {
            char *proces;
            char *polecenie;
            char *pomocnicza;
            char rozdzielacz[] = " ";
            char terminal[60];
            scanf("%s", terminal);
            proces = strtok(terminal, rozdzielacz);
            polecenie = strtok(NULL, rozdzielacz);
            pomocnicza = strtok(NULL, rozdzielacz);

            // wyciagamy ID konfiguracyjne dla procesu
            ID_kolejki = zwroc_ID(proces);
            // otwieramy jej kanal komunikacji
            int msgid_2 = msgget(ID_kolejki, IPC_CREAT | 0640);
            if (msgid_2 == -1)
            {
                perror("Blad tworzenia drugiej kolejki\n");
                exit(1);
            }

            // ID pomocniczej kolejki
            int ID_pomocniczej_kolejki = atoi(pomocnicza);
            // otwieram jej kanal komunikacji
            int msgid_pomocnicza = msgget(ID_pomocniczej_kolejki, IPC_CREAT | 0640);
            if (msgid_pomocnicza == -1)
            {
                perror("Blad tworzenia pomocniczej kolejki\n");
                exit(1);
            }

            msgbuff m;
            m.mtype = 1;
            strcpy(m.mtext, terminal);
            // wysylam do kolejki od zczytanego procesu to co wprowadzono w terminal
            if (msgsnd(msgid_2, &m, (sizeof(msgbuff) - sizeof(long)), 0) == -1)
            {
                perror("Wysylanie polecenia powiodlo sie\n");
                exit(1);
            }

            // oczekujemy na wynik z POMOCNICZEJ kolejki
            int rozmiar_komunikatu_z_pomocniczej = 0;

            rozmiar_komunikatu_z_pomocniczej = msgrcv(msgid_pomocnicza, &m, (sizeof(msgbuff) - sizeof(long)), 1, 0);
            if (rozmiar_komunikatu_z_pomocniczej == -1)
            {
                perror("Blad odbierania");
                exit(1);
            }
            // odczytujemy wynik polecenia z parametru #2 wykonanego w procesie z parametru #1
            if (rozmiar_komunikatu_z_pomocniczej > 0)
            {
                printf("Odebrano: %s", m.mtext);
            }
        }
        break;
    }
    }
}