#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>

#include "proto.h"
static int msgid;

void handler(int s)
{
    msgctl(msgid, IPC_RMID, NULL);
}

int main()
{
    __sighandler_t old=signal(SIGINT, handler);
    key_t key;
    struct msg_st rbuf;
    key = ftok(KEYPATH, KEYPROJ);
    if(key<0)
    {
        perror("ftok()");
        exit(1);
    }

    msgid= msgget(key, IPC_CREAT | 0600);
    if(msgid<0)
    {
        perror("msgget()");
        exit(1);
    }
    while(1)
    {
        //消息中包的大小信息需要减去
        if(msgrcv(msgid, &rbuf, sizeof(rbuf) - sizeof(long), 0, 0)<0)
        {
            perror("msgrcv()");
            exit(1);
        }
        printf("NAME=%s\n", rbuf.name);
        printf("CHINESE : %d ", rbuf.chinese);
        printf("MATH : %d\n",rbuf.math);
    }
    signal(SIGINT, old);
    exit(0);
}