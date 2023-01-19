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
#include <time.h>
#include <stdbool.h>

typedef struct msg_polecenie
{
    long mtype;
    char pomocnicza[7];
    char polecenie[60];
} msg_polecenie;

typedef struct msg_wynik
{
    long mtype;
    char wynik[2048];
} msg_wynik;

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
    // printf("ID odczytane z pliku %d\n", ID_kolejki);
    if (ID_kolejki == 0)
    {
        perror("Nie udalo sie odczytac ID kolejki z pliku konfiguracyjnego\n");
        exit(1);
    }

    fclose(f_konfig);
    return ID_kolejki;
}

char **podziel_tekst(char input[], int *licznik)
{
    char **wynik = malloc(50 * sizeof(char *));
    for (int i = 0; i < 50; i++)
    {
        wynik[i] = malloc(50 * sizeof(char));
    }
    int biezacy_indeks = 0;
    licznik = malloc(sizeof(int));
    *licznik = 0;
    printf("%d", *licznik);
    for (int i = 0; i < strlen(input); i++)
    {
        char biezacy_znak = input[i];
        bool w_cudzyslowie = false;
        if (biezacy_znak == '\"')
        {
            w_cudzyslowie = !w_cudzyslowie;
        }
        else if (!w_cudzyslowie && biezacy_znak == ' ')
        {
            biezacy_indeks = 0;
            *licznik++;
        }
        else if (biezacy_znak != '\n') // albo inny konczacy linie, nw
        {
            wynik[*licznik][biezacy_indeks++] = biezacy_znak;
        }
    }
    printf("%d", *licznik);
    return wynik;
}

int policz_spacje(char terminal[])
{
    int ilosc_spacji = 0;
    for (int i = 0; i < strlen(terminal); i++)
    {
        if (terminal[i] == ' ')
        {
            ilosc_spacji++;
        }
    }

    return ilosc_spacji;
}
void obsluz_macierzysty_proces()
{
    while (1)
    {
        char *proces;
        char rozdzielacz[] = " ";
        char terminal[100];
        char *polecenie = terminal;
        char *pomocnicza;

        fgets(terminal, 50, stdin);
        // printf("terminal: %s\n", terminal);

        // w petli robie od drugiego do konca rozdzielajac spacjami ale laczac je strcatem
        // używam strtok_r aby mieć dostęp do pozostałej części inputa
        proces = strtok_r(polecenie, rozdzielacz, &polecenie);
        // printf("proces: %s\n", proces);

        char *token;
        char *kopia = malloc(sizeof(char) * strlen(polecenie));
        strcpy(kopia, polecenie);

        while ((token = strtok_r(kopia, rozdzielacz, &kopia)) != NULL)
        {
            pomocnicza = token;
        };

        // \0 - koniec stringa
        // obcinam w ten sposób niepotrzebne znaki na końcu
        pomocnicza[strlen(pomocnicza) - 1] = '\0';
        polecenie[strlen(polecenie) - strlen(pomocnicza) - 1] = '\0';
        // printf("polecenie: %s\n", polecenie);
        // printf("pomocnicza: %s\n", pomocnicza);

        // printf("polecenie: %s\npomocnicza: %s\n", polecenie, pomocnicza);
        // printf("polecenie strlen: %d\npomocnicza strlen: %d\n", strlen(polecenie), strlen(pomocnicza));

        // wyciagamy ID konfiguracyjne dla procesu
        int ID_kolejki_2 = zwroc_ID(proces);
        // otwieramy jej kanal komunikacji
        int msgid_2 = msgget(ID_kolejki_2, IPC_CREAT | 0640);
        // printf("ID kanalu komunikacji dla tego procesu %d\n", msgid_2);
        if (msgid_2 == -1)
        {
            perror("Blad tworzenia drugiej kolejki\n");
            exit(1);
        }
        // ID pomocniczej kolejki
        int ID_pomocniczej_kolejki = atoi(pomocnicza);

        // otwieram jej kanal komunikacji
        int msgid_pomocnicza = msgget(ID_pomocniczej_kolejki, IPC_CREAT | 0640);
        // printf("ID kanalu komunikacji dla pomocniczej kolejki %d\n", msgid_pomocnicza);
        if (msgid_pomocnicza == -1)
        {
            perror("Blad tworzenia pomocniczej kolejki\n");
            exit(1);
        }

        msg_polecenie m;
        m.mtype = 1;
        strcpy(m.polecenie, polecenie);
        strcpy(m.pomocnicza, pomocnicza);
        // printf("tekst w m.pomocnicza: %s\n", m.pomocnicza);
        // printf("tekst w m.polecenie: %s\n", m.polecenie);

        // wysylam do kolejki od zczytanego procesu to co wprowadzono w terminal
        if (msgsnd(msgid_2, &m, (sizeof(msg_polecenie) - sizeof(long)), 0) == -1)
        {
            perror("Wysylanie polecenia nie powiodlo sie\n");
            exit(1);
        }

        // oczekujemy na wynik z POMOCNICZEJ kolejki
        int rozmiar_komunikatu_z_pomocniczej = 0;
        // printf("rozmiar komunikatu z pomocniczej %d\n", rozmiar_komunikatu_z_pomocniczej);

        msg_wynik w;
        rozmiar_komunikatu_z_pomocniczej = msgrcv(msgid_pomocnicza, &w, (sizeof(msg_wynik) - sizeof(long)), 1, 0);
        if (rozmiar_komunikatu_z_pomocniczej == -1)
        {
            perror("Blad odbierania");
            exit(1);
        }
        // odczytujemy wynik polecenia z parametru #2 wykonanego w procesie z parametru #1
        if (rozmiar_komunikatu_z_pomocniczej > 0)
        {
            printf("\nOdebrano: \n%s", w.wynik);
            printf("\n");
        }
    }
}

void obsluz_potomny_proces(int msgid_1)
{
    while (1)
    {
        // sprawdzam kolejkę dla mojego procesu (np. tutaj odpalony był usr1 więc sprawdzam czy inny proces tu cos nie wpisal)
        msg_polecenie m;
        int rozmiar_komunikatu = 0;
        // printf("UWAGA BEDE SPRAWDZAU ZAPNIJ PASY\n");
        rozmiar_komunikatu = msgrcv(msgid_1, &m, (sizeof(msg_polecenie) - sizeof(long)), 1, 0);
        if (rozmiar_komunikatu == -1)
        {
            perror("Blad odbierania");
            exit(1);
        }

        // 2. jak coś jest to wczytuje polecenie i nazwe kolejki pomocniczej - inaczej petla leci od poczatku
        if (rozmiar_komunikatu > 0)
        {
            // printf("Odebrano m.pomocnicza: %s\n", m.pomocnicza);
            // printf("Odebrano m.polecenie: %s\n", m.polecenie);

            int pipefd[2];

            pipe(pipefd);
            pid_t pid = fork();

            if (pid == 0)
            {
                char *polecenie[50];
                char delimiter[] = " \n";
                char *buffer = strtok(m.polecenie, delimiter);
                polecenie[0] = buffer;
                int i = 0;
                printf("%s\n", polecenie[0]);
                for (i = 1; buffer != NULL; i++)
                {
                    buffer = strtok(NULL, delimiter);
                    polecenie[i] = buffer;
                    // printf("%s\n", polecenie[i]);
                }

                // przekierowanie wyjscia procesu na stdout
                close(pipefd[0]);
                close(STDOUT_FILENO);
                dup(pipefd[1]);
                execvp(polecenie[0], polecenie);
            }

            close(pipefd[1]);
            int status;
            waitpid(pid, &status, 0);
            printf("Status procesu wykonujacego polecenie: %d\n", status);
            int rozmiar = 1000;
            char wynik[8192];
            int ilosc_przeczytanych_bajtow = 0;
            char tablica_na_przeczytane_litery[rozmiar];

            // czytam z deskryptora procseu potomnego
            // printf("ZACZYNAM CZYTANIE\n");
            while ((ilosc_przeczytanych_bajtow = read(pipefd[0], tablica_na_przeczytane_litery, rozmiar)) > 0)
            {
                strcpy(wynik, tablica_na_przeczytane_litery);
            }
            close(pipefd[0]);

            if (ilosc_przeczytanych_bajtow == -1)
            {
                printf("Blad czytania wyniku procesu\n");
            }
            int ID_pomocniczej_kolejki = atoi(m.pomocnicza);

            // 4. wynik polecenia wpisuje do kolejki pomocniczej
            int msgid_pomocnicza = msgget(ID_pomocniczej_kolejki, IPC_CREAT | 0640);
            if (msgid_pomocnicza == -1)
            {
                perror("Blad tworzenia pomocniczej kolejki\n");
                exit(1);
            }

            msg_wynik w;
            w.mtype = 1;
            strcpy(w.wynik, wynik);
            printf("tekst w w.wynik %s\n", w.wynik);

            // wysylam do kolejki od zczytanego procesu wynik polecenia wprowadzonego w terminal
            if (msgsnd(msgid_pomocnicza, &w, (sizeof(msg_wynik) - sizeof(long)), 0) == -1)
            {
                perror("Wysylanie wyniku nie powiodlo sie\n");
                exit(1);
            }

            break;
        }
    }
}

int main(int argc, char *argv[])
{
    // srand(time(NULL));

    int ID_kolejki = zwroc_ID(argv[1]);
    int msgid_1 = msgget(ID_kolejki, IPC_CREAT | 0640);
    if (msgid_1 == -1)
    {
        perror("Blad tworzenia pierwszej kolejki\n");
        exit(1);
    }
    // printf("msgid %d\n", msgid_1);

    pid_t pid = fork();
    switch (pid)
    {
    case 0: // dla potomnego
    {
        obsluz_potomny_proces(msgid_1);
        break;
    }
    default: // dla macierzystego
    {
        // printf("PID POTOMNEGO: %d\n", pid);
        obsluz_macierzysty_proces();
        break;
    }
    }

    // printf("\nJEBANE DZIALA\n");
}