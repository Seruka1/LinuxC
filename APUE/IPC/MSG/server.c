#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>

#include "proto.h"

int main()
{
    key_t key;
    struct msg_st sbuf;
    key = ftok(KEYPATH, KEYPROJ);
    int msgid=msgget(key,0);

    sbuf.mtype = 1;
    strcpy(sbuf.name, "Alan");
    sbuf.chinese = rand() % 100;
    sbuf.math = rand() % 100;

    msgsnd(msgid, &sbuf, sizeof(sbuf) - sizeof(long), 0);
    puts("ok!");
    exit(0);
}