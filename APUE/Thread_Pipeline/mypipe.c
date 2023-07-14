#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "mypipe.h"


struct mypipe_st
{
    int head;
    int tail;
    char* buf[PIPESIZE]; 
    int datasize;
    int count_reader;
    int count_writer;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

mypipe_t *mypipe_init()
{
    struct mypipe_st* pipe;
    pipe=malloc(sizeof(*pipe));
    if(pipe==NULL)
    {
        return NULL;
    }
    pipe->head=0;
    pipe->tail=0;
    pipe->datasize=0;
    pipe->count_reader=0;
    pipe->count_writer=0;
    pthread_mutex_init(&pipe->mutex,NULL);
    pthread_cond_init(&pipe->cond,NULL);
}

// 读者 写者 注册身份
int mypipe_register(mypipe_t *ptr, int opmap)
{
    struct mypipe_st* pipe=ptr;
    pthread_mutex_lock(&pipe->mutex);
    if(opmap&PIPE_READER)
    {
        ++pipe->count_reader;
    }
    if(opmap&PIPE_WRITER)
    {
        ++pipe->count_writer;
    }
    pthread_cond_broadcast(&pipe->cond);
    // 读写双方不全
    while(pipe->count_reader<=0||pipe->count_writer<=0)
    {
        pthread_cond_wait(&pipe->cond,&pipe->mutex);
    }

    pthread_mutex_unlock(&pipe->mutex);
    return 0;
}

// 读者 写者 注销身份
int mypipe_unregister(mypipe_t *ptr, int opmap)
{
    struct mypipe_st* pipe=ptr;
    pthread_mutex_lock(&pipe->mutex);
    if(opmap&PIPE_READER)
    {
        --pipe->count_reader;
    }
    if(opmap&PIPE_WRITER)
    {
        --pipe->count_writer;
    }
    // 唤醒其他管道读写方检查读写者的数量
    pthread_cond_broadcast(&pipe->cond);
    pthread_mutex_unlock(&pipe->mutex);
    return 0;
}

int mypipe_read_unlock(struct mypipe_st* pipe,char* cha)
{
    if(--pipe->datasize<0)
    {
        ++pipe->datasize;
        return -1;
    }
    *cha=pipe->buf[pipe->head];
    pipe->head=(pipe->head+1)%PIPESIZE;
    return 0;
}


int mypipe_read(mypipe_t *ptr, void *buf, size_t size)
{
    struct mypipe_st* pipe=ptr;
    pthread_mutex_lock(&pipe->mutex);
    while(pipe->datasize<=0&&pipe->count_writer>0)
    {
        pthread_cond_wait(&pipe->cond,&pipe->mutex);
    }
    if(pipe->datasize<=0&&pipe->count_writer<=0)
    {
        pthread_mutex_unlock(&pipe->mutex);
        return 0;
    }

    int s=size>pipe->datasize?pipe->datasize:size;
    for(int i=0;i<s;++i)
    {
       if( mypipe_read_unlock(pipe,buf+i)<0)
       {
            break;
       }
    }

    pthread_mutex_unlock(&pipe->mutex);
    return 0;
}

int mypipe_write_unlock(struct mypipe_st* pipe,const char* cha)
{
    if(++pipe->datasize>PIPESIZE)
    {
        --pipe->datasize;
        return -1;
    }
    pipe->buf[pipe->tail]=*cha;
    pipe->tail=(pipe->tail+1)%PIPESIZE;

    return 0;
}

int mypipe_write(mypipe_t *ptr, const void *buf, size_t size)
{
    struct mypipe_st* pipe=ptr;
    pthread_mutex_lock(&pipe->mutex);
    while(pipe->datasize>=PIPESIZE&&pipe->count_reader>0)
    {
        pthread_cond_wait(&pipe->cond,&pipe->mutex);
    }
    if(pipe->datasize>=PIPESIZE&&pipe->count_reader<=0)
    {
        pthread_mutex_unlock(&pipe->mutex);
        return 0;
    }
    int s=size>(PIPESIZE-pipe->datasize)?PIPESIZE-pipe->datasize:size;
    for(int i=0;i<s;++i)
    {
        if(mypipe_write_unlock(pipe,buf+i)<0)
        {
            break;
        }
    }
    pthread_mutex_unlock(&pipe->mutex);
    return 0;
}



int mypipe_destory(mypipe_t *ptr)
{
    struct mypipe_st* pipe=ptr;
    pthread_mutex_destroy(&pipe->mutex);
    pthread_cond_destroy(&pipe->cond);
    free(pipe);
    return 0;
}