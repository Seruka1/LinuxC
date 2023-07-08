#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include "mytbf.h"

// 令牌桶
#define BUFSIZE 1024
#define CPS 10
#define BURST 100





int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage...");
        exit(1);
    }
    int sfd;
    
    mytbf_t * tbf=mytbf_init(CPS,BURST);
    if(tbf==NULL)
    {
        fprintf(stderr,"mytbf_init failed!\n");
        exit(1);
    }
    do
    {
        sfd = open(argv[1], O_RDONLY);
        if(sfd<0)
        {
            if(errno!=EINTR)
            {
                perror("open()");
                exit(1);
            }
        }
    } while (sfd<0);
    
    int dfd = 1;

    int size;
    char buf[BUFSIZE];

    while (1)
    {
        size=mytbf_fetchtoken(tbf,BUFSIZE);
        if(size<0)
        {
            fprintf(stderr,"mytbf_fetchtoken%s",strerror(-size));
            exit(1);
        }
        int len = 0;
        int pos = 0;
        int ret = 0;
        int fg = 0;
        while ((len = read(sfd, buf, size)) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            printf("%s\n", strerror(errno));
            fg = 1;
            break;
        }
        if (len == 0 || fg == 1)
        {
            break;
        }

        if(size-len>0)
        {
            mytbf_returntoken(tbf,size-len);
        }

        while (len > 0)
        {
            ret = write(dfd, buf + pos, len);
            if (ret < 0)
            {
                if(errno==EINTR)
                {
                    continue;
                }
                printf("%s\n", strerror(errno));
                // 注意这里直接退出会发生内存泄漏
                exit(1);
            }
            pos += ret;
            len -= ret;
        }
        // sleep在不同操作系统上的实现不一样
        // sleep(1);
    }

    close(sfd);
    mytbf_destroy(tbf);
    exit(0);
}