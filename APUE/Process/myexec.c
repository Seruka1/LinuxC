#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int main()
{
    puts("Begin()!");
    //这里不刷新的话，如果将输出终端重定向可能会在exec把缓冲区清空
    fflush(NULL);
    execl("/bin/date","date","+%s",NULL);
    //正常情况下不会执行下面命令了。
    perror("execl()");
    puts("End!");
    exit(0);
}