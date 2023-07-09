#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void int_handler(int s)
{
    write(1, "!", 1);
}

int main()
{
    // signal(SIGINT,SIG_IGN);
    sigset_t set, oldset, saveset;

    // 信号会打断阻塞的系统调用
    signal(SIGINT, int_handler);

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_UNBLOCK, &set, &saveset);
    sigprocmask(SIG_BLOCK, &set, &oldset);
    for (int j = 0; j < 1000; ++j)
    {
        
        for (int i = 0; i < 5; ++i)
        {
            write(1, "*", 1);
            sleep(1);
        }
        write(1, "\n", 1);
        sigsuspend(&oldset);
    }
    sigprocmask(SIG_SETMASK, &saveset, NULL);
    exit(0);
}