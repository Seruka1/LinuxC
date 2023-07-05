#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int main()
{
    pid_t pid;
    printf("[%d]:Begin!\n",getpid());
    fflush(NULL); //fork之前一定要刷新缓冲区，不然缓冲区会复制一份到子进程中，导致输出两次
    pid=fork();
    if(pid<0)
    {
        perror("fork()");
        exit(1);
    }
    else if(pid==0) //child
    {
        printf("[%d]:Child is working!\n",getpid());
    }
    else //parent
    {
        printf("[%d]:Parent is working!\n",getpid());
    }
    
    printf("End!\n");
    exit(0);
}