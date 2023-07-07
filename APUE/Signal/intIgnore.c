#include<stdio.h>
#include<stdlib.h>
#include<signal.h>

void int_handler(int s)
{
    write(1,"!",1);
}

int main()
{
    int i;

    //signal(SIGINT,SIG_IGN);

    //信号会打断阻塞的系统调用
    signal(SIGINT,int_handler);

    for(int i=0;i<10;++i)
    {
        write(1,"*",1);
        sleep(1);
    }
    exit(0);
}