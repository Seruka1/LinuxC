#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <wait.h>
#include <errno.h>
#define BUFSIZE 1024

int main()
{
    char buf[BUFSIZE];
    int pd[2];
    pid_t pid;
    if(pipe(pd)<0)
    {
        perror("pipe()");
        exit(1);
    }
    pid = fork();
    if(pid<0)
    {
        perror("fork()");
        exit(1);
    }
    if(pid==0)  //child read
    {
        close(pd[1]);
        int len = read(pd[0], buf, BUFSIZE);
        write(1, buf, len);
        close(pd[0]);
        exit(0);
    }
    else 
    {
        close(pd[0]);
        write(pd[1], "Hello!", 6);
        close(pd[1]);
        wait(NULL);
        exit(0);
    }
}