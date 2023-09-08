#include "lst_timer.h"
#include "../http/http_conn.h"
util_timer::util_timer():prev(nullptr),next(nullptr){}

sort_timer_lst::sort_timer_lst():head(nullptr),tail(nullptr){}


// 链表被销毁时，删除所有的定时器
sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp = head;
    while(tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

// 将目标定时器添加到链表中
void sort_timer_lst::add_timer(util_timer *timer)
{

    if (!timer)
    {
        return;
    }
    if (!head) // 添加的为第一个节点，头结点（尾节点）
    {
        head = tail = timer;
    }
    // 目标定时器的超时时间最小，则把该定时器插入链表头部,作为链表新的头节点
    else if(timer->expire<head->expire)
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
    }
    // 否则调用重载函数，把它插入head节点之后合适的位置，以保证链表的升序特性
    else 
    {
        add_timer(timer, head);
    }
}

/* 当某个任务发生变化时，定时器状态发生变化，需要调整定时器在链表中的位置。这个函数只考虑被调整的定时器的
超时时间延长的情况，即该定时器需要往链表的尾部移动。 */
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    if(!tmp||timer->expire<tmp->expire)
    {
        return;
    }
    // 如果目标定时器是链表的头节点，则将该定时器从链表中取出并重新插入链表
    if(timer==head)
    {
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer, head);
    }
    else
    {
        // 如果目标定时器不是链表的头节点，则将该定时器从链表中取出，然后插入其原来所在位置后的部分链表中
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

// 将目标定时器移除
void sort_timer_lst::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    // 下面这个条件成立表示链表中只有一个定时器，即目标定时器
    if(head==timer&&tail==timer)
    {
        head = nullptr;
        tail = nullptr;
        delete timer;
    }
    /* 如果链表中至少有两个定时器，且目标定时器是链表的头节点，
    则将链表的头节点重置为原头节点的下一个节点，然后删除目标定时器。 */
    else if(head==timer)
    {
        head = head->next;
        head->prev = nullptr;
        delete timer;
    }
    /* 如果链表中至少有两个定时器，且目标定时器是链表的尾节点，
   则将链表的尾节点重置为原尾节点的前一个节点，然后删除目标定时器。*/
    else if(tail==timer)
    {
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
    }
    // 如果目标定时器位于链表的中间，则把它前后的定时器串联起来，然后删除目标定时器
    else
    {
        timer->next->prev = timer->prev;
        timer->prev->next = timer->next;
        delete timer;
    }
}

// SIGALARM 信号每次触发时在其信号处理函数中执行一次tick()函数，处理到期任务
void sort_timer_lst::tick()
{
    if (!head)
    {
        return;
    }

    // 从前往后处理,直到遇到一个尚未到期的定时器
    util_timer *tmp = head;
    time_t cur_time = time(NULL);
    while (tmp && tmp->expire<=cur_time)
    {
        // 调用定时器的回调函数，以执行定时任务，关闭连接
        tmp->cb_func(tmp->user_data);
        del_timer(tmp);
        tmp = head;
    }
}

// 辅助函数 被共有的add_timer函数与adjust_timer函数调用，该函数将目标定时器插入到lst_head后的部分链表中
void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head)
{
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    /* 遍历 list_head 节点之后的部分链表，直到找到一个超时时间大于目标定时器的超时时间节点
   并将目标定时器插入该节点之前 */
    while (tmp)
    {
        if(timer->expire<tmp->expire)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            return;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    /* 如果遍历完 lst_head 节点之后的部分链表，仍未找到超时时间大于目标定时器的超时时间的节点，
        则将目标定时器插入链表尾部，并把它设置为链表新的尾节点。*/
    if(!tmp)
    {
        prev->next = timer;
        timer->prev = tail;
        timer->next = nullptr;
        tail = timer;
    }
}



//Utils工具类
int Utils::setnonblocking(int fd)
{
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd=fd;
    if(1==TRIGMode)
    {
        event.events=EPOLLIN|EPOLLOUT|EPOLLRDHUP;
    }
    else
    {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if(one_shot)
    {
        event.events|=EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg=sig;
    send(u_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler=handler;
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_list.tick();
    alarm(m_TIMESLOT);
}

//用于连接不上（如文件描述符到达上限时）时，向客户端发生错误信息
void Utils::show_error(int connfd, const char *info)
{
    send(connfd,info,sizeof(info),0);
    close(connfd);
}

int *Utils::u_pipefd=0;
int Utils::u_epollfd=0;

class Utils;

void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}