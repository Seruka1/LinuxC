#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

static void cleanup_func(void * p)
{
    puts(p);
}

static void* func(void* p)
{
    puts("Thread is working!");

    pthread_cleanup_push(cleanup_func,"cleanup:1");
    pthread_cleanup_push(cleanup_func,"cleanup:2");
    pthread_cleanup_push(cleanup_func,"cleanup:3");

    puts("push over!");
    
    //pop输入0表示不载入对应的钩子函数

    //pthread_exit(NULL);
    //下面的内容执行不到但是不会报错 会按照全为 1 处理
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}

int main()
{
    puts("Begin!");

    pthread_t tid;
    int err=pthread_create(&tid,NULL,func,NULL);
    if(err!=0)
    {
        fprintf(stderr,"pthread_create(): %s\n",strerror(err));
        exit(1);
    }

    pthread_join(tid,NULL);
    puts("End!");
    exit(0);

}