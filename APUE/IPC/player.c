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
    if (pipe(pd) < 0)
    {
        perror("pipe()");
        exit(1);
    }
    pid = fork();
    if (pid < 0)
    {
        perror("fork()");
        exit(1);
    }
    if (pid == 0) // child read
    {
        close(pd[1]);
        dup2(pd[1], 0);
        close(pd[1]);
        int fd = open("/dev/null", O_RDWR);
        dup2(fd, 1);
        dup2(fd, 2);
        execl("/usr/bin/mpg123", "mpg123", "-", NULL);
        perror("execl()");
        exit(1);
    }
    else
    {
        close(pd[0]);
        //父进程从网络上收数据
        close(pd[1]);
        wait(NULL);
        exit(0);
    }
}