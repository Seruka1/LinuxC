#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <errno.h>

#include "proto.h"

#define SIG_NOTIFY SIGUSR2
#define MINSPACESERVER 5
#define MAXSPACESERVER 10
#define MAXCILENT 20
#define IPSIZE 40
#define BUFSIZE 1024
// 父进程维护进程池（通过信号），子进程进行工作
enum
{
    STATE_IDLE = 0,
    STATE_BUSY
};

struct server_st
{
    pid_t pid;
    int state;
};

static struct server_st *serverpool;
static int idle_count = 0, busy_count = 0;
static int sfd; // socket描述符

void notify_handler(int s)
{
    return;
}

void scan_pool()
{
    int idle = 0, busy = 0;
    for (int i = 0; i < MAXCILENT; ++i)
    {
        if (serverpool[i].pid == -1)
        {
            continue;
        }
        // 检测进程是否存在
        if (kill(serverpool[i].pid, 0))
        {
            serverpool[i].pid = -1;
            continue;
        }
        if (serverpool[i].state == STATE_BUSY)
        {
            ++busy;
        }
        else if (serverpool[i].state == STATE_IDLE)
        {
            ++idle;
        }
        else
        {
            fprintf(stderr, "未知错误!\n");
            abort();
        }
    }
    idle_count = idle;
    busy_count = busy;
}

void server_job(int slot)
{
    int newsd;
    struct sockaddr_in raddr;
    int ppid = getppid();
    socklen_t raddr_len = sizeof(raddr);
    char ip[IPSIZE];
    while (1)
    {
        serverpool[slot].state = STATE_IDLE;
        kill(ppid, SIG_NOTIFY);
        newsd = accept(sfd, (void *)&raddr, &raddr_len);
        if (newsd < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            else
            {
                perror("accept()");
                exit(1);
            }
        }
        serverpool[slot].state = STATE_BUSY;
        kill(ppid, SIG_NOTIFY);
        inet_ntop(AF_INET, &raddr.sin_addr, ip, IPSIZE);
        char buf[BUFSIZE];
        int pkglen = 0;

        pkglen = sprintf(buf, FMT_STAMP, (long long)time(NULL));

        if (send(newsd, buf, pkglen, 0) < 0)
        {
            perror("send()");
            exit(1);
        }

        close(newsd);
        sleep(5);
    }
}

int add_1_server()
{
    if (idle_count + busy_count >= MAXCILENT)
    {
        return -1;
    }
    int slot;
    pid_t pid;
    for (slot = 0; slot < MAXCILENT; slot++)
    {
        if (serverpool[slot].pid == -1)
        {
            break;
        }
    }
    serverpool[slot].state = STATE_IDLE;
    pid = fork();
    if (pid < 0)
    {
        perror("fork()");
        exit(1);
    }
    if (pid == 0)
    {
        server_job(slot);
        exit(0);
    }
    else
    {
        serverpool[slot].pid = pid;
        idle_count++;
    }
    return 0;
}

int del_1_server()
{
    if (idle_count <= 0)
    {
        return -1;
    }
    for (int i = 0; i < MAXCILENT; ++i)
    {
        if (serverpool[i].pid != -1 && serverpool[i].state == STATE_IDLE)
        {
            kill(serverpool[i].pid, SIGTERM);
            serverpool[i].pid = -1;
            --idle_count;
            break;
        }
    }
    return 0;
}

int main()
{
    // 处理信号
    struct sigaction sa, osa;
    sa.sa_handler = SIG_IGN; // 忽略父进程的资源回收信号
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT; // 让子进程结束后自行消亡，不会变为僵尸状态
    sigaction(SIGCLD, &sa, &osa);

    sa.sa_handler = notify_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIG_NOTIFY, &sa, &osa);

    // 设置信号屏蔽集
    sigset_t sigset, oldsigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIG_NOTIFY);
    sigprocmask(SIG_BLOCK, &sigset, &oldsigset);

    serverpool = mmap(NULL, sizeof(struct server_st) * MAXCILENT, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (serverpool == MAP_FAILED)
    {
        perror("mmap()");
        exit(1);
    }
    // 初始化进程池 pid=-1表示未被使用
    for (int i = 0; i < MAXCILENT; ++i)
    {
        serverpool[i].pid = -1;
    }

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
    {
        perror("socket()");
        exit(1);
    }
    int val = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    } // 复用socket地址

    struct sockaddr_in laddr;
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    if (bind(sfd, (void *)&laddr, sizeof(laddr)) < 0)
    {
        perror("bind()");
        exit(1);
    }

    if (listen(sfd, 100) < 0)
    {
        perror("listen()");
        exit(1);
    }
    for (int i = 0; i < MINSPACESERVER; ++i)
    {
        add_1_server();
    }

    while (1)
    {
        sigsuspend(&oldsigset);
        scan_pool();
        if (idle_count < MINSPACESERVER)
        {
            for (int i = 0; i < MINSPACESERVER - idle_count; ++i)
            {
                add_1_server();
            }
        }
        else if (idle_count > MAXSPACESERVER)
        {
            for (int i = 0; i < idle_count - MAXSPACESERVER; ++i)
            {
                del_1_server();
            }
        }
        // printf pool
        for (int i = 0; i < MAXCILENT; ++i)
        {
            if (serverpool[i].pid == -1)
            {
                putchar(' ');
            }
            else if (serverpool[i].state == STATE_IDLE)
            {
                putchar('.');
            }
            else
            {
                putchar('x');
            }
            fflush(NULL);
        }
        putchar('\n');
    }
    close(sfd);
    sigprocmask(SIG_NOTIFY, &oldsigset, NULL);
    // 恢复信号屏蔽集
    // 恢复信号默认处理
    exit(0);
}