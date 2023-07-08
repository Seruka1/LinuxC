#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include"anytimer.h"
static struct task *tasks[JOB_MAX];
static __sighandler_t ori;



struct task
{
    int sec;
    at_jobfunc_t* job;
    char* s;
    int flag;
};

void alam_handler(int s)
{
    alarm(1);
    for (int i = 0; i < JOB_MAX; ++i)
    {
        if (tasks[i] == NULL || tasks[i]->flag == 0)
        {
            continue;
        }
        if (--tasks[i]->sec == 0)
        {
            tasks[i]->job(tasks[i]->s);
        }
    }
}

int getpos()
{
    for (int i = 0; i < JOB_MAX;++i)
    {
        if(tasks[i]==NULL)
        {
            return i;
        }
    }
    return -1;
}

int at_canceljob(int id)
{
    free(tasks[id]);
    tasks[id] = NULL;
    return 0;
}

int at_addjob(int sec, at_jobfunc_t *jobp, void *arg)
{
    if(sec<=0)
    {
        return -EINVAL;
    }
    int pos = getpos();
    if(pos<0)
    {
        return -ENOSPC;
    }

    struct task *curTask;
    curTask = malloc(sizeof(*curTask));
    if(curTask==NULL)
    {
        return -ENOMEM;
    }
    curTask->sec = sec;
    curTask->job = jobp;
    curTask->s = arg;
    curTask->flag = 1;
    tasks[pos] = curTask;
    return pos;
}

void jobStart()
{
    ori=signal(SIGALRM, alam_handler);
    alarm(1);
}

void jobEnd()
{
    signal(SIGALRM, ori);
    alarm(0);
    for (int i = 0; i < JOB_MAX;++i)
    {
        at_canceljob(i);
    }
}

static void f1(void* p)
{
    printf("f1():%s\n", p);
}
static void f2(void *p)
{
    printf("f2():%s\n", p);
}
static void f3(void *p)
{
    printf("f3():%s\n", p);
}

int main()
{
    int job1, job2, job3;
    puts("Begin!");
    job1=at_addjob(5, f1, "aaa");
    if(job1<0)
    {
        fprintf(stderr, "at_addjob():%s\n", strerror(-job1));
    }
    job2=at_addjob(2, f2, "bbb");
    if (job2 < 0)
    {
        fprintf(stderr, "at_addjob():%s\n", strerror(-job2));
    }
    job3=at_addjob(7, f3, "ccc");
    if (job3 < 0)
    {
        fprintf(stderr, "at_addjob():%s\n", strerror(-job3));
    }
    puts("End!");
    jobStart();
    while (1)
    {
        write(1, ".", 1);
        sleep(1);
    }

    exit(0);
}