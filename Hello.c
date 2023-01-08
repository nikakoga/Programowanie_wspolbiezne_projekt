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

	int msgid = msgget(12, IPC_CREAT | 0640);
	printf("ID KOLEJKI TO %d\n", msgid);

	if (fork() == 0)
	{
		msgbuff m;
		m.mtype = 1;
		strcpy(m.mtext, "Hello!");
		if (msgsnd(msgid, &m, (sizeof(msgbuff) - sizeof(long)), 0) == -1)
		{
			perror("Wysylanie");
			exit(1);
		}
	}

	else
	{
		msgbuff m;
		int spr = msgrcv(msgid, &m, (sizeof(msgbuff) - sizeof(long)), 1, 0);
		if (spr == -1)
		{
			perror("Odbieranie");
			exit(1);
		}
		printf("Odebrano: %s", m.mtext);
	}
}
