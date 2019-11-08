#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
int *result;
key_t key = 189633;
int total = 0;
int dofib(int n)
{
    if(n==0)
        return 0;
    else if(n==1)
        return 1;
    else
        return (dofib(n-1)+dofib(n-2));
}

int sharedMemoryFib(int n, int len)
{
    int shmid;
	int status;
    shmid = shmget(key, len * sizeof(int), IPC_CREAT | 0666);
    if(shmid < 0)
    {
        printf("Error: SHMGET in sharedMemoryFib\n");
        return 0;
    }
    result = (int*)(shmat(shmid, NULL, 0));
    if(result == (int *) -1)
    {
        printf("Error: SHMAT");
        return 0;
    }
	if(n>1)
	{
		if(result[n-1]==-1)
		{
			pid_t pid;
			pid = fork();
			if(pid==0)
				sharedMemoryFib((n-1), len);
			else if(pid>0)
			{
				wait(&status);
			}
            if(pid>0)
                pid = fork();
			if(pid==0)
				sharedMemoryFib((n-2),len);
			else if(pid>0)
			{
				wait(&status);
			}
        }
		else if(result[n-1]!=-1)
		{
			result[n] = result[n-1]+result[n-2];
		}
	}
	return 0;

}
void pipesFib(int n, int doPrint, int pipeDescriptor)
{
    int val = 0;
    int val2 = 0;
    int resultPipe = 0;
    int fd1[2];
    int fd2[2];
    pipe(fd1);
    pipe(fd2);
    if(n>1)
    {
        pid_t pid;
        int status;
        pid = fork();
        if(pid == 0)
        {
            close(fd1[0]);
            pipesFib(n-1,0,fd1[1]);
            exit(0);

        }
        if(pid>0)
        {
            pid = fork();
            if(pid==0)
            {
                close(fd2[0]);
                pipesFib(n-2,0,fd2[1]);
                exit(0);
            }
            else
            {
                wait(&status);
                wait(&status);
                close(fd1[1]);
                close(fd2[1]);
                read(fd1[0],&val,sizeof(val));
                read(fd2[0],&val2,sizeof(val2));
                resultPipe = val+val2;
            }
        }
    }
    else
    {
        resultPipe = n;
        write(pipeDescriptor,&resultPipe, sizeof(resultPipe));
    }
    
    if(doPrint==1)
    {
        printf("%d\n",resultPipe);
    }
    else
    {
        write(pipeDescriptor,&resultPipe,sizeof(resultPipe));
    }
}
int main(int argc, char **argv)
{
    int num = 0;
	if(argc<2)
	{
		printf("No arguments given\n");
		return 0;
	}
    else
    {
        for(int i=0;i<strlen(argv[1]);i++)
            num = num * 10 + ((int)argv[1][i]-'0');
		if(num>13)
		{
			printf("Error: the number is greater than 13\n");
			return 0;
		}
        pipesFib(num,1,0);
        num += 2;
		int shmid;
        shmid = shmget(key, num * sizeof(int), IPC_CREAT | 0666);
		if(shmid < 0)
		{
			printf("Error: SHMGET change variable key and try again\n");
			return 0;
		}
        result = (int*)(shmat(shmid, NULL, 0));
        if(result == (int *) -1)
        {
            printf("Error: SHMAT");
            return 0;
        }
		for(int x=0;x<num;x++)
		{
			result[x] = -1;
		}
		result[0] = 0;
		result[1] = 1;
        
        int status;
		pid_t pid;
		pid = fork();
		if(pid<0)
		{
			printf("PID failed\n");
			return 0;
		}
        else if(pid==0)
        {
            sharedMemoryFib((num),num);
        }
		else if(pid>0)
		{
            wait(&status);
            printf("%d\n", result[num-2]);
		}
    }

	return 0;
}
