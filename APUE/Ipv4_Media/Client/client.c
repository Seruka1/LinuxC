#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <fcntl.h>
#include <wait.h>
#include <errno.h>
#include <getopt.h>

#include "../Include/proto.h"
//#include "proto.h"
#include "client.h"

/*
 * -M --mgroup  指定多播组
 * -P --port    指定接收端口
 * -p --player  指定播放器
 * -H --help    显示帮助
 */

struct client_conf_st client_conf = {
    .rcvport = DEFAULT_RCVPORT,
    .mgroup=DEFAULT_MGROUP,
    .player_cmd=DEFAULT_PLAYERCMD
};

static void printhelp()
{
    printf("-P --port     指定接收端口\n");
    printf("-M --mgroup   指定多播组\n");
    printf("-p --player   指定播放器\n");
    printf("-H --help     显示帮助\n");
}

static int writen(int fd,const char* buf,size_t len)
{
    int ret;
    int pos = 0;
    while (len>0)
    {
        ret=write(fd, buf+pos, len);
        if(ret<0)
        {
            if(errno==EINTR)
            {
                continue;
            }
            perror("write()");
            return -1;
        }
        len -= ret;
        pos += ret;
    }
    return 0;
}

int main(int argc,char** argv)
{
    int index = 0;
    struct option argarr[]={
        {"port",1,NULL,'P'},
        {"mgroup",1,NULL,'M'},
        {"player",1,NULL,'p'},
        {"help",1,NULL,'H'},
        {NULL,0,NULL,0}
    };

    //命令行解析
    while(1)
    {
        int c= getopt_long(argc,argv,"P:M:p:H",argarr,&index);
        if(c<0)
        {
            break;
        }
        switch(c)
        {
            case 'P':
            {
            client_conf.rcvport = optarg;
            break;
            }
            case 'M':
            {
            client_conf.mgroup = optarg;
            break;
            }
            case 'p':
            {
            client_conf.player_cmd = optarg;
            break;
            }
            case 'H':
            {
            printhelp();
            break;
            }
            default:
            {
            abort();
            break;
            }
        }
    }
    int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sd<0)
    {
        perror("socket()");
        exit(1);
    }
    struct ip_mreqn mreq;
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
    inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
    mreq.imr_ifindex = if_nametoindex(DEFAULT_IF_NAME);

    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }
    int val = 1;
   
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &val, sizeof(val)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }

    struct sockaddr_in laddr;
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(client_conf.rcvport));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr.s_addr);
    if(bind(sd,(void*)&laddr,sizeof(laddr))<0)
    {
        perror("bind()");
        exit(1);
    }
    int pd[2];
    if(pipe(pd)<0)
    {
        perror("pipe()");
        exit(1);
    }
    pid_t pid = fork();
    if(pid<0)
    {
        perror("fork()");
        exit(1);
    }
    if(pid==0)  //子进程  调用解码器
    {
        close(sd);
        close(pd[1]);
        dup2(pd[0], 0);
        if(pd[0]>0)
        {
            close(pd[0]);
        }
        if (execl("/bin/sh", "sh", "-c" ,client_conf.player_cmd, NULL) < 0)
        {
            perror("execl()");
            exit(1);
        }
    }
    else
    {
        //父进程   从socket接收包，通过管道发给子进程
        close(pd[0]);

        //接收节目单包
        struct msg_list_st *msg_list;
        msg_list = malloc(MSG_LIST_MAX);
        if(msg_list==NULL)
        {
            perror("malloc()");
            exit(1);
        }
        struct sockaddr_in serveraddr;
        socklen_t serveraddr_len = sizeof(serveraddr);
        int len;
        while (1)
        {
            len=recvfrom(sd, msg_list, MSG_LIST_MAX,0, (void *)&serveraddr, &serveraddr_len);
            if(len<sizeof(struct msg_list_st))
            {
                fprintf(stderr, "message is too small\n");
                continue;
            }
            if(msg_list->chnid!=LISTCHNID)
            {
                continue;
            }
            break;
        }
        //打印节目单
        struct msg_listentry_st *pos;
        for (pos = msg_list->entry; (char *)pos < (((char *)msg_list) + len);pos=(void*)(((char*)pos)+ntohs(pos->len)))
        {
            printf("channel %d : %s\n", pos->chnid, pos->desc);
        }
        free(msg_list);
        puts("Please choose a channel:");
        int ret;
        int chosenid;
        do
        {
            ret = scanf("%d", &chosenid);
        } while (ret < 1);

        fprintf(stdout, "chosen channel = %d\n", chosenid);
        
        //接受频道数据包
        struct msg_channel_st* msg_channel;
        msg_channel = malloc(MSG_CHANNEL_MAX);
        if(msg_channel==NULL)
        {
            perror("malloc()");
            exit(1);
        }

        struct sockaddr_in raddr;
        socklen_t raddr_len = sizeof(raddr);
        while(1)
        {
            len = recvfrom(sd, msg_channel, MSG_CHANNEL_MAX, 0, (void *)&raddr, &raddr_len);
            if(raddr.sin_addr.s_addr!=serveraddr.sin_addr.s_addr||raddr.sin_port!=serveraddr.sin_port)
            {
                fprintf(stderr, "Ignore:address not match.\n");
                continue;
            }
            if(len<sizeof(struct msg_channel_st))
            {
                fprintf(stderr, "message is too small.\n");
                continue;
            }
            if(msg_channel->chnid==chosenid)
            {
                // 可考虑设置缓存区接收字节，避免音乐播放断断续续
                fprintf(stdout, "accepted channel %d msg.\n", msg_channel->chnid);
                //读取数据 写入pd[1]
                if(writen(pd[1],msg_channel->data,len-sizeof(chnid_t))<0)
                {
                    fprintf(stderr, "write(): write data error()\n");
                    exit(1);
                }
            }
        }

        free(msg_channel);
        close(sd);
        wait(NULL);
    }
    exit(0);
}
