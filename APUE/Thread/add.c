#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

#define FNAME "/tmp/out"
#define THRNUM 20
#define LINESIZE 1024
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void* handler(void *)
{
    FILE *fp = fopen(FNAME, "r+");
    if(fp==NULL)
    {
        perror("fopen()");
        exit(1);
    }
    char buf[LINESIZE];

    pthread_mutex_lock(&mut);
    fgets(buf, LINESIZE, fp);
    fseek(fp, 0, SEEK_SET);
    //sleep(1);
    fprintf(fp, "%d\n", atoi(buf) + 1);
    fclose(fp);
    pthread_mutex_unlock(&mut);

    pthread_exit(NULL);
}

int main()
{
    pthread_t Ptid[THRNUM];
    int err;

    for (int i = 0; i < THRNUM; ++i)
    {
        err = pthread_create(&Ptid[i], NULL, handler, NULL);
        if (err)
        {
            perror("pthread_create():");
            exit(1);
        }
    }

    for (int i = 0; i < THRNUM; ++i)
    {
        pthread_join(Ptid[i],NULL);
    }

    pthread_mutex_destroy(&mut);
    exit(0);
}
