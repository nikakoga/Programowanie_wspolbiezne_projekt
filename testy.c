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

int main()
{

    char *jeden = "jeden";
    char *dwa = "dwa";

    typedef struct msgbuff
    {
        long mtype;
        char mtext[7];
    } msgbuff;

    msgbuff m;
    m.mtype = 1;
    strcpy(m.mtext, jeden);

    strcpy(m.mtext, jeden);
    strcpy(m.mtext, dwa);
    printf("%s", m.mtext);
}
