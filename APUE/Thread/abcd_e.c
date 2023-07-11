#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

// 太妙了  4把锁实现轮流打印
#define THRNUM 4

static int num = 0;
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int next(int n)
{
    if (n + 1 == THRNUM)
    {
        return 0;
    }
    return n + 1;
}

static void *handler(void *p)
{
    int n = (int)p;
    int c = 'a' + n;
    while(1)
    {
        pthread_mutex_lock(&mut);
        while (n != num)
        {
            pthread_cond_wait(&cond, &mut);
        }
        write(1, &c, 1);
        num = next(num);
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mut);
    }
    pthread_exit(NULL);
}

int main()
{
    pthread_t Ptid[THRNUM];
    int err;

    for (int i = 0; i < THRNUM; ++i)
    {
        err = pthread_create(&Ptid[i], NULL, handler, (void *)i);
        if (err)
        {
            perror("pthread_create():");
            exit(1);
        }
    }
    alarm(5);

    for (int i = 0; i < THRNUM; ++i)
    {
        pthread_join(Ptid[i], NULL);
    }
    pthread_mutex_destroy(&mut );
    pthread_cond_destroy(&cond);

    exit(0);
}
