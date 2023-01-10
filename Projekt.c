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

typedef struct msgbuff
{
    long mtype;
    char mtext[7];
} msgbuff;

int randomint(int min, int max)
{
    int tmp;
    if (max >= min)
        max -= min;
    else
    {
        tmp = min - max;
        min = max;
        max = tmp;
    }
    if (max)
    {
        return rand() % max + min;
    }
    else
    {
        return min;
    }

    // z geeks for geeks
    // rand() % (max - min + 1)) + min
}

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
    printf("ID odczytane z pliku %d\n", ID_kolejki);
    if (ID_kolejki == 0)
    {
        perror("Nie udalo sie odczytac ID kolejki z pliku konfiguracyjnego\n");
        exit(1);
    }

    fclose(f_konfig);
    return ID_kolejki;
}

void obsluz_macierzysty_proces_kolego()
{
    while (1)
    {
        char *proces;
        char polecenie[] = "";
        char *czesc_polecenia;
        // char *pomocnicza;
        char rozdzielacz[] = " ";
        char terminal[60];
        fgets(terminal, 50, stdin);
        printf("terminal: %s\n", terminal);

        // liczenie ile spacji jest w pliku
        int ilosc_spacji = 0;
        for (int i = 0; i < strlen(terminal); i++)
        {
            if (terminal[i] == ' ')
            {
                ilosc_spacji++;
            }
        }

        printf("ilosc spacji: %d\n", ilosc_spacji);

        // w petli robisz od drugiego do przedostatniego rozdzielajac spacjami ale laczac je strcatem
        proces = strtok(terminal, rozdzielacz);
        printf("proces: %s\n", proces);
        for (int i = 0; i < ilosc_spacji - 1; i++)
        {
            czesc_polecenia = strtok(NULL, rozdzielacz);
            strcat(polecenie, czesc_polecenia);
        }

        if (ilosc_spacji == 1) // to jest sytuacja kiedy cale polecenie to np : ls
        {
            strcat(polecenie, strtok(NULL, rozdzielacz));
        }

        printf("polecenie: %s\n", polecenie);
        // pomocnicza = strtok(NULL, rozdzielacz);
        // printf("pomocnicza: %s\n", pomocnicza);

        // wyciagamy ID konfiguracyjne dla procesu
        int ID_kolejki_2 = zwroc_ID(proces);
        // otwieramy jej kanal komunikacji
        int msgid_2 = msgget(ID_kolejki_2, IPC_CREAT | 0640);
        printf("ID kanalu komunikacji dla tego procesu %d\n", msgid_2);
        if (msgid_2 == -1)
        {
            perror("Blad tworzenia drugiej kolejki\n");
            exit(1);
        }
        // ID pomocniczej kolejki
        int ID_pomocniczej_kolejki = randomint(100000, 999999);

        // otwieram jej kanal komunikacji
        int msgid_pomocnicza = msgget(ID_pomocniczej_kolejki, IPC_CREAT | 0640);
        printf("ID kanalu komunikacji dla pomocniczej kolejki %d\n", msgid_pomocnicza);
        if (msgid_pomocnicza == -1)
        {
            perror("Blad tworzenia pomocniczej kolejki\n");
            exit(1);
        }

        msgbuff m;
        m.mtype = 1;
        strcat(m.mtext, polecenie);
        strcat(m.mtext, " ");
        char id_str[6];
        sprintf(id_str, "%d", ID_pomocniczej_kolejki);
        strcat(m.mtext, id_str);
        printf("tekst w m.text %s\n", m.mtext);

        // wysylam do kolejki od zczytanego procesu to co wprowadzono w terminal
        if (msgsnd(msgid_2, &m, (sizeof(msgbuff) - sizeof(long)), 0) == -1)
        {
            perror("Wysylanie polecenia nie powiodlo sie\n");
            exit(1);
        }

        // oczekujemy na wynik z POMOCNICZEJ kolejki
        int rozmiar_komunikatu_z_pomocniczej = 0;
        printf("rozmiar komunikatu z pomocniczej %d\n", rozmiar_komunikatu_z_pomocniczej);

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
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    int ID_kolejki = zwroc_ID(argv[1]);
    int msgid_1 = msgget(ID_kolejki, IPC_CREAT | 0640);
    if (msgid_1 == -1)
    {
        perror("Blad tworzenia pierwszej kolejki\n");
        exit(1);
    }
    printf("msgid %d\n", msgid_1);

    switch (fork())
    {
    case 0: // dla potomnego
    {
        while (1)
        {
            // sprawdzam kolejkę dla mojego procesu (np. tutaj odpalony był usr1 więc sprawdzam czy inny proces tu cos nie wpisal)
            msgbuff m;
            int rozmiar_komunikatu = 0;
            printf("UWAGA BEDE SPRAWDZAU ZAPNIJ PASY\n");
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
                printf("proces potomny terminal %s\n", terminal);

                char *proces;
                char *polecenie;
                char *pomocnicza;
                char rozdzielacz[] = " ";
                proces = strtok(terminal, rozdzielacz);
                printf("proces: %s\n", proces);
                polecenie = strtok(NULL, rozdzielacz);
                printf("polecenie: %s\n", proces);
                pomocnicza = strtok(NULL, rozdzielacz);
                printf("pomocnicza: %s\n", proces);
                int ID_pomocniczej_kolejki = atoi(pomocnicza);

                // tworze plik ktory bedzie zapisywac rzeczy z wyjscia ktore to przekaze do kolejki komunikatow
                int plik_pomocniczy = creat("wyjscie.txt", O_RDWR);
                if (plik_pomocniczy == -1)
                {
                    perror("Blad tworzenia pliku na wynik\n");
                    exit(1);
                }

                switch (fork())
                {
                case 0:
                {
                    dup2(plik_pomocniczy, 1); // teraz pisze do pliku zamiast na wyjscie
                    // 3. wykonuje polecenie

                    break;
                }

                default:
                {
                    // do zmiennej wynik zczytuje to co jest w pliku "wyjscie.txt"
                    int rozmiar = 1000;
                    char *wynik;
                    int ilosc_przeczytanych_bajtow = 0;
                    char tablica_na_przeczytane_litery[rozmiar];

                    while ((ilosc_przeczytanych_bajtow = read(plik_pomocniczy, wynik, rozmiar)) > 0)
                    {
                        strcpy(wynik, tablica_na_przeczytane_litery);
                    }

                    if (ilosc_przeczytanych_bajtow == -1)
                    {
                        printf("Blad czytania pliku z wynikiem\n");
                    }

                    // 4. wynik polecenia wpisuje do kolejki pomocniczej
                    int msgid_pomocnicza = msgget(ID_pomocniczej_kolejki, IPC_CREAT | 0640);
                    if (msgid_pomocnicza == -1)
                    {
                        perror("Blad tworzenia pomocniczej kolejki\n");
                        exit(1);
                    }

                    msgbuff m;
                    m.mtype = 1;
                    strcat(m.mtext, wynik);
                    printf("tekst w m.text %s\n", m.mtext);

                    // wysylam do kolejki od zczytanego procesu to co wprowadzono w terminal
                    if (msgsnd(msgid_pomocnicza, &m, (sizeof(msgbuff) - sizeof(long)), 0) == -1)
                    {
                        perror("Wysylanie wyniku nie powiodlo sie\n");
                        exit(1);
                    }
                    break;
                }
                }

                close(plik_pomocniczy);
            }
        }

        break;
    }
    default: // dla macierzystego
    {
        obsluz_macierzysty_proces_kolego();
        break;
    }
    }

    printf("\nJEBANE DZIALA\n");
}