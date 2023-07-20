#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <sys/types.h>
#include <unistd.h>

#include "anytimer.h"

static void f1(void *s)
{
    printf("f1():%s\n", (char *)s);
    fflush(NULL);
}

static void f2(void *s)
{
   printf("f2():%s\n", (char *)s);
    fflush(NULL);
}

static void repeat(void)
{
    int job1;

    puts("Begin!");
    job1 = at_addjob_repeat(2, f1, "aaa");
    if(job1 < 0)
    {
        fprintf(stderr, "at_addjob():%s\n", strerror(-job1));
        exit(1);
    }
    while(1) 
    {
        write(1, ".", 1);
        sleep(1);
    }
}

static void jobalarm(void)
{
    int job1, job2, job3;

    puts("Begin!");
    job1 = at_addjob(5, f1, "aaa");
    if(job1 < 0)
    {
        fprintf(stderr, "at_addjob():%s\n", strerror(-job1));
        exit(1);
    }

    at_addjob(2, f2, "bbb");
    at_addjob(7, f1, "ccc");

    while(1) 
    {
        write(1, ".", 1);
        sleep(1);
    }
}

int main(void)
{
     repeat();
    //jobalarm();

    return 0;
}