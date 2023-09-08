#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"


class util_timer;


struct client_data
{
    sockaddr_in address;     //地址主要是用来记录日志的
    int sockfd;
    util_timer *timer;
};

class util_timer
{
public:
    util_timer();

public:
    time_t expire;          //任务超时时间，这里使用绝对时间

    //函数指针，在外部进行函数的定义
    void (* cb_func)(client_data*);    //回调函数  超时时触发，epoll del，关闭连接，移除定时器
    client_data *user_data; //任务数据(任务数组是以sockfd为下标的)
    util_timer *prev;       //前一个指针
    util_timer *next;       //后一个指针
};


//定时器升序双向链表，用于处理过时定时器以及更新定时器的数据结构。
class sort_timer_lst
{
public:
    sort_timer_lst();

    //链表被销毁时，删除所有的定时器
    ~sort_timer_lst();

    //将目标定时器添加到链表中
    void add_timer(util_timer *timer);

    /* 当某个任务发生变化时，定时器状态发生变化，需要调整定时器在链表中的位置。这个函数只考虑被调整的定时器的
    超时时间延长的情况，即该定时器需要往链表的尾部移动。 */
    void adjust_timer(util_timer *timer);

    //将目标定时器移除
    void del_timer(util_timer *timer);

    //SIGALARM 信号每次触发时在其信号处理函数中执行一次tick()函数，处理到期任务
    void tick();

private:

    //辅助函数 被共有的add_timer函数与adjust_timer函数调用，该函数将目标定时器插入到lst_head后的部分链表中
    void add_timer(util_timer *timer, util_timer *lst_head);

private:
    util_timer *head; // 头节点
    util_timer *tail;  //尾节点
};

//工具类，帮主线程干脏活
class Utils
{
public:
    Utils(){}
    ~Utils(){}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    //用于连接不上（如文件描述符到达上限时）时，向客户端发生错误信息
    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    sort_timer_lst m_timer_list;
    static int u_epollfd;
    int m_TIMESLOT;
};

//回调函数
void cb_func(client_data *user_data);

#endif