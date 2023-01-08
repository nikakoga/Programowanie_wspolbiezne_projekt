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

int main(int argc, char *argv[])
{

	typedef struct msgbuff
	{
		long mtype;
		char mtext[7];
	} msgbuff;

	int msgid = msgget(1336, IPC_CREAT | 0640);

	if (fork() == 0)
	{
		msgbuff m;
		strcpy(m.mtext, "Hello!");
		for (int i = 1; i <= 10; i++)
		{
			m.mtype = i;
			printf("%d", i);
			if (msgsnd(msgid, &m, (sizeof(msgbuff) - sizeof(long)), 0) == -1)
			{
				perror("Wysylanie");
				exit(1);
			}
		}
	}

	else
	{
		msgbuff m;
		int wybranytyp = atoi(argv[1]);
		if (msgrcv(msgid, &m, (sizeof(msgbuff) - sizeof(long)), wybranytyp, 0) == -1)
		{
			perror("Odbieranie");
			exit(1);
		}
		printf("Odebrano: %s %ld", m.mtext, m.mtype);
	}
}
