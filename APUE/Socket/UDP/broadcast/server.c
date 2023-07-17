#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>

#include "proto.h"

#define IPSIZE 1024

int main()
{
    int sfd;
    struct sockaddr_in laddr; // local addr
    struct sockaddr_in raddr; // remote addr
    struct msg_st *rbuf;
    char ip[IPSIZE];

    int pkglen = sizeof(struct msg_st) + NAMEMAX;
    rbuf = malloc(pkglen);
    if (rbuf == NULL)
    {
        perror("malloc()");
        exit(1);
    }
    sfd = socket(AF_INET, SOCK_DGRAM, 0 /*IPPROTO_UDP*/);
    if (sfd < 0)
    {
        perror("socket()");
        exit(1);
    }
    //打开socket广播选项
    int val = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }

    laddr.sin_family = AF_INET;                     // 指定协议
    laddr.sin_port = htons(atoi(SERVERPORT));       // 指定网络通信端口
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr); // IPv4点分式转二进制数
    if (bind(sfd, (void *)&laddr, sizeof(laddr)) < 0)
    {
        perror("bind()");
        exit(1);
    }
    /*!!!必须初始化 不然第一次Server端读到的地址是错的*/
    socklen_t raddr_len = sizeof(raddr);
    while (1)
    {
        recvfrom(sfd, rbuf, pkglen, 0, (void *)&raddr, &raddr_len); // 报式套接字每次通信都需要知道对方是谁
        inet_ntop(AF_INET, &raddr.sin_addr, ip, IPSIZE);
        printf("%s %d\n", ip, ntohs(raddr.sin_port));
        printf("%s %d %d\n", rbuf->name, ntohl(rbuf->math), ntohl(rbuf->chinese));
        fflush(NULL);
    }

    close(sfd);

    exit(0);
}