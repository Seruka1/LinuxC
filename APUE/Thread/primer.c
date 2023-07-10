#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

#define N 7
#define LEFT 30000000
#define RIGHT 30000200

struct thr_arg_st
{
    int n;
};

static void * handler(void *p)
{
    int num = ((struct thr_arg_st *)p)->n;
    for (int i = LEFT + num; i < RIGHT;i+=N)
    {
        int mark = 1;
        for (int j = 2; j <= i/2;++j)
        {
            if(i%j==0)
            {
                mark = 0;
                break;
            }
        }
        if(mark==1)
        {
            printf("%d is primer[%d]\n", i, num);
        }
    }
    pthread_exit(p);
} 

// 交叉算法计算 池类算法涉及到竞争
int main()
{
    pthread_t Ptid[N];
    void *ptr = NULL;
    int err;

    for (int i = 0; i < N;++i)
    {
        struct thr_arg_st *p;
        p = malloc(sizeof(*p));
        p->n = i;
        err=pthread_create(&Ptid[i], NULL, handler, p);
        if(err)
        {
            perror("pthread_create():");
            exit(1);
        }
    }

    for (int i = 0; i < N;++i)
    {
        pthread_join(Ptid[i], ptr);
        free(ptr);
    }
    exit(0);
}
