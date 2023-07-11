#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include<pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "mytbf.h"


//原来是通过信号每秒给job加token，现在改为一个线程每秒给job加token
static pthread_t tid_alrm;
static struct mytbf_st *job[MYTBF_MAX];
static pthread_mutex_t mut_job=PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t init_once=PTHREAD_ONCE_INIT;

struct mytbf_st
{
    int csp;
    int burst;
    int token;
    int pos;
    pthread_mutex_t mut;
};


static int get_free_pos_unlocked()
{
    for (int i = 0; i < MYTBF_MAX; i++)
    {
        if (job[i] == NULL)
            return i;
    }
    return -1;
}

static void* thr_alrm(void* p)
{
    while(1)
    {
        pthread_mutex_lock(&mut_job);
        for(int i=0;i<MYTBF_MAX;++i)
        {
            if(job[i]==NULL)
            {
                continue;
            }
            pthread_mutex_lock(&job[i]->mut);
            job[i]->token+=job[i]->csp;
            if(job[i]->token>job[i]->burst)
            {
                job[i]->token=job[i]->burst;
            }
            pthread_mutex_unlock(&job[i]->mut);
        }
        pthread_mutex_unlock(&mut_job);
        sleep(1);
    }
}


static void mod_unload(void)
{
    pthread_cancel(tid_alrm);
    pthread_join(tid_alrm,NULL);
    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        if(job[i]!=NULL)
        {
            mytbf_destroy(job[i]);
        }
    }
    pthread_mutex_destroy(&mut_job);
}

static void mod_load(void)
{
    int err;
    err=pthread_create(&tid_alrm,NULL,thr_alrm,NULL);
    if(err)
    {
        fprintf(stderr,"pthread_create():%s\n",strerror(err));
        exit(1);
    }
    atexit(mod_unload);
}

mytbf_t *mytbf_init(int cps, int burst)
{

    pthread_once(&init_once,mod_load);

    struct mytbf_st *me;
    me = malloc(sizeof(*me));
    if (me == NULL)
        return NULL;
    me->token = 0;
    me->csp = cps;
    me->burst = burst;
    pthread_mutex_init(&me->mut,NULL);

    pthread_mutex_lock(&mut_job);
    int pos = get_free_pos_unlocked();
    if (pos < 0)
    {
        pthread_mutex_unlock(&mut_job);
        free(me);
        return NULL;
    }
    me->pos = pos;
    job[pos] = me;
    pthread_mutex_unlock(&mut_job);
    return me;
}

// 获取token
int mytbf_fetchtoken(mytbf_t *ptr, int size)
{
    struct mytbf_st *tbf = ptr;

    if (size <= 0)
    {
        return -EINVAL;
    }

    // 有token继续
    pthread_mutex_lock(&tbf->mut);
    while (tbf->token <= 0)
    {
        pthread_mutex_unlock(&tbf->mut);
        sched_yield();
        pthread_mutex_lock(&tbf->mut);
    }

    int n = tbf->token < size ? tbf->token : size;

    tbf->token -= n;
    pthread_mutex_unlock(&tbf->mut);
    // 用户获取了 n 个token
    return n;
}
// 归还token
int mytbf_returntoken(mytbf_t *ptr, int size)
{
    struct mytbf_st *tbf = ptr;

    if (size <= 0)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&tbf->mut);
    tbf->token += size;
    if (tbf->token > tbf->burst)
        tbf->token = tbf->burst;
    pthread_mutex_unlock(&tbf->mut);
    return size;
}

int mytbf_destroy(mytbf_t *ptr)
{
    struct mytbf_st *tbf = ptr;
    pthread_mutex_lock(&mut_job);
    job[tbf->pos] = NULL;
    pthread_mutex_unlock(&mut_job);
    pthread_mutex_destroy(&tbf->mut);
    free(tbf);
    return 0;
}