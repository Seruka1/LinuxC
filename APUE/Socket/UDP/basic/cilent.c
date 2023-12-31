#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "proto.h"




int main()
{
    int sfd;
    struct msg_st *sbuf;
    struct sockaddr_in raddr; // remote addr
    // bind
    sfd= socket(AF_INET, SOCK_DGRAM, 0);
    int pkglen = sizeof(struct msg_st) + strlen("Mike") + 1;
    sbuf = malloc(pkglen);
    if (sbuf == NULL)
    {
        perror("malloc()");
        exit(1);
    }
    char *name = "Mike";
    strcpy(sbuf->name, name);
    sbuf->math = htonl(rand() % 100); // 主机字节序转网络字节序
    sbuf->chinese = htonl(rand() % 100);
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &raddr.sin_addr);
    if (sendto(sfd, sbuf, pkglen, 0, (void *)&raddr, sizeof(raddr)) < 0)
    {
        perror("sendto()");
        exit(1);
    }
    puts("OK");

    close(sfd);

    exit(0);
}