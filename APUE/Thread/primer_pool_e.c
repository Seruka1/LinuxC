#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

#define N 4
#define LEFT 30000000
#define RIGHT 30000200
// 线程池 main发送任务，线程能者多劳
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int num = 0;

static void *handler(void *p)
{
    int n = (int)p;
    while (1)
    {
        pthread_mutex_lock(&mut);
        while (num == 0)
        {
            pthread_cond_wait(&cond, &mut);
        }
        if (num == -1)
        {
            pthread_mutex_unlock(&mut);
            break;
        }
        int mark = 1;
        for (int j = 2; j <= num / 2; ++j)
        {
            if (num % j == 0)
            {
                mark = 0;
                break;
            }
        }
        if (mark == 1)
        {
            printf("%d is primer[%d]\n", num, n);
        }

        num = 0;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mut);
    }
    pthread_exit(NULL);
}

// 交叉算法计算 池类算法涉及到竞争
int main()
{
    pthread_t Ptid[N];

    int err;

    for (int i = 0; i < N; ++i)
    {
        err = pthread_create(&Ptid[i], NULL, handler, (void *)i);
        if (err)
        {
            perror("pthread_create():");
            exit(1);
        }
    }

    for (int i = LEFT; i <= RIGHT; ++i)
    {
        pthread_mutex_lock(&mut);
        while (num != 0)
        {
            pthread_cond_wait(&cond, &mut);
        }
        num = i;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mut);
    }
    pthread_mutex_lock(&mut);
    while (num != 0)
    {
        pthread_cond_wait(&cond, &mut);
    }
    num = -1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mut);

    for (int i = 0; i < N; ++i)
    {
        pthread_join(Ptid[i], NULL);
    }
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&cond);
    exit(0);
}
