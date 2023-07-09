#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "mytbf.h"

//static __sighandler_t ori;
static struct sigaction ori;


struct mytbf_st
{
    int csp;
    int burst;
    int token;
    int pos;
};

static struct mytbf_st *job[MYTBF_MAX];

static volatile int inited = 0;

static int get_free_pos()
{
    for (int i = 0; i < MYTBF_MAX; i++)
    {
        if (job[i] == NULL)
            return i;
    }
    return -1;
}

static void alrm_action(int s,siginfo_t* infop,void* unused)
{
    //alarm(1);
    //判断信号来源是不是kernel
    if(infop->si_code!=SI_KERNEL)
    {
        return;
    }
    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        if (job[i] == NULL)
        {
            continue;
        }
        job[i]->token += job[i]->csp;
        {
            if (job[i]->token > job[i]->burst)
            {
                job[i]->token = job[i]->burst;
            }
        }
    }
}

static void mod_unload(void)
{
//    signal(SIGALRM, ori);
//    alarm(0);
    struct itimerval itv;

    sigaction(SIGALRM, &ori, NULL);

    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);

    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        free(job[i]);
    }
}

static void mod_load(void)
{
//    ori = signal(SIGALRM, alrm_handler);
//   alarm(1);
    struct sigaction sa;
    struct itimerval itv;
    sa.sa_sigaction = alrm_action;
    sigemptyset(&sa.sa_flags);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGALRM, &sa, &ori);
    /*if error*/

    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);
    /*if error*/

    atexit(mod_unload);
}

mytbf_t *mytbf_init(int cps, int burst)
{
    if (inited == 0)
    {
        mod_load();
        inited = 1;
    }
    struct mytbf_st *me;
    me = malloc(sizeof(*me));
    if (me == NULL)
        return NULL;
    int pos = get_free_pos();
    if (pos < 0)
    {
        return NULL;
    }
    me->token = 0;
    me->csp = cps;
    me->burst = burst;
    me->pos = pos;
    job[pos] = me;

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
    while (tbf->token <= 0)
    {
        pause();
    }

    int n = tbf->token < size ? tbf->token : size;

    tbf->token -= n;
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

    tbf->token += size;
    if (tbf->token > tbf->burst)
        tbf->token = tbf->burst;

    return size;
}

int mytbf_destroy(mytbf_t *ptr)
{
    struct mytbf_st *tbf = ptr;
    job[tbf->pos] = NULL;
    free(tbf);
    return 0;
}