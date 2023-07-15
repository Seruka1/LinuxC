#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <wait.h>
#include <errno.h>

#define THRNUM 20
#define FNAME "/tmp/out"
#define BUFSIZE 1024
static int semid;
static void P()
{
    struct sembuf op;
    op.sem_num=0;
    op.sem_op=-1;
    op.sem_flg=0;
    while(semop(semid,&op,1)<0)
    {
        if(errno!=EINTR||errno!=EAGAIN)
        {
            perror("semop()");
            exit(1);
        }
    }
}

static void V()
{
    struct sembuf op;
    op.sem_num=0;
    op.sem_op=1;
    op.sem_flg=0;
    while(semop(semid,&op,1)<0)
    {
        if(errno!=EINTR||errno!=EAGAIN)
        {
            perror("semop()");
            exit(1);
        }
    }
}

static void handler()
{
    FILE *fp = fopen(FNAME, "r+");
    char buf[BUFSIZE];
    if (fp == NULL)
    {
        perror("fopen()");
        exit(1);
    }
    P();
    fgets(buf, BUFSIZE, fp);
    fseek(fp, 0, SEEK_SET);
    sleep(1);
    fprintf(fp, "%d", atoi(buf) + 1);
    fflush(fp);
    V();
    fclose(fp);
}

int main()
{
    pid_t pid;
    
    //父子进程直接继承segid
    semid=semget(IPC_PRIVATE,1,0600);
    if(semid<0)
    {
        perror("semget()");
        exit(1);
    }

    if(semctl(semid,0,SETVAL,1)<0)
    {
        perror("semctl()");
        exit(1);
    }

    for (int i = 0; i < THRNUM;++i)
    {
        pid = fork();
        if(pid<0)
        {
            perror("fork()");
            exit(1);
        }
        if(pid==0)
        {
            handler();
            exit(0);
        }
    }
    for (int i = 0; i < THRNUM;++i)
    {
        wait(NULL);
    }
    semctl(semid,0,IPC_RMID);
    return 0;
}
