#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

// 令牌桶
#define SIZE 10
#define BUCKETS 100

static int token = 0;

static void alrm_handler(int s)
{
    alarm(1);
    if (token <= BUCKETS)
        ++token;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stdout, "Usage...");
        exit(1);
    }
    int sfd = open(argv[1], O_RDONLY);
    if (sfd < 0)
    {
        strerror(errno);
        exit(1);
    }
    int dfd = 1;

    signal(SIGALRM, alrm_handler);
    alarm(1);

    char buf[SIZE];

    while (1)
    {
        //注意下面这条操作不是原子操作会影响实际token值
        while (token <= 0)
        {
            pause();
        }
        --token;
        int len = 0;
        int pos = 0;
        int ret = 0;
        int fg = 0;
        while ((len = read(sfd, buf, SIZE)) < 0)
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
        while (len > 0)
        {
            ret = write(dfd, buf + pos, len);
            if (ret < 0)
            {
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

    exit(0);
}