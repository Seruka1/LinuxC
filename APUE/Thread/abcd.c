#include<stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

//太妙了  4把锁实现轮流打印
#define THRNUM 4

static pthread_mutex_t mut[THRNUM] ;

static int next(int n)
{
    if(n+1==THRNUM)
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
        pthread_mutex_lock(mut + n);
        write(1, &c, 1);
        pthread_mutex_unlock(mut + next(n));
    }
    pthread_exit(NULL);
}

int main()
{
    pthread_t Ptid[THRNUM];
    int err;

    for (int i = 0; i < THRNUM; ++i)
    {
        pthread_mutex_init(mut+i,NULL);
        pthread_mutex_lock(mut + i);
        err = pthread_create(&Ptid[i], NULL, handler, (void *)i);
        if (err)
        {
            perror("pthread_create():");
            exit(1);
        }
    }
    alarm(5);
    pthread_mutex_unlock(mut);
   
    for (int i = 0; i < THRNUM; ++i)
    {
        pthread_join(Ptid[i], NULL);
    }
    for (int i = 0; i < THRNUM;++i)
    {
        pthread_mutex_destroy(mut+i);
    }
    exit(0);
}
