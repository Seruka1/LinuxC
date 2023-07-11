#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

#include "mysem.h"

#define N 4
#define LEFT 30000000
#define RIGHT 30000200
#define THRNUM (RIGHT-LEFT+1)
static mysem_t *sem;

static void *handler(void *p)
{
    int i = (int)p;

    int mark = 1;
    for (int j = 2; j <= i / 2; ++j)
    {
        if (i % j == 0)
        {
            mark = 0;
            break;
        }
    }
    if (mark == 1)
    {
        printf("%d is primer\n", i);
    }
    mysem_add(sem, 1);
    pthread_exit(NULL);
}

// 交叉算法计算 池类算法涉及到竞争
int main()
{
    pthread_t Ptid[THRNUM];
    int err;
    sem = mysem_init(N);
    if (sem == NULL)
    {
        fprintf(stderr, "mysem_init():failed!");
    }
    for (int i = LEFT; i <=RIGHT; ++i)
    {
        mysem_sub(sem, 1);
        err = pthread_create(Ptid + (i - LEFT), NULL, handler, (void *)i);
        if (err)
        {
            perror("pthread_create():");
            exit(1);
        }
    }

    for (int i = 0; i <THRNUM; ++i)
    {
        pthread_join(Ptid [i], NULL);
    }
    mysem_destory(sem);
    exit(0);
}
