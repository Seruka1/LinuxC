#include"lst_timer.h"

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
    EMlog(LOGLEVEL_DEBUG, "===========adding timer.=============\n");
    if (!timer)
    {
        EMlog(LOGLEVEL_WARN, "===========timer null.=========\n");
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
    EMlog(LOGLEVEL_DEBUG, "===========added timer.==========\n");
}

/* 当某个任务发生变化时，定时器状态发生变化，需要调整定时器在链表中的位置。这个函数只考虑被调整的定时器的
超时时间延长的情况，即该定时器需要往链表的尾部移动。 */
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    EMlog(LOGLEVEL_DEBUG, "===========adjusting timer.=============\n");
    if (!timer)
    {
        EMlog(LOGLEVEL_WARN, "===========timer null.=========\n");
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
    EMlog(LOGLEVEL_DEBUG, "===========adjusted timer.==========\n");
}

// 将目标定时器移除
void sort_timer_lst::del_timer(util_timer *timer)
{
    EMlog(LOGLEVEL_DEBUG, "===========deleting timer.===========\n");
    if (!timer)
    {
        // http_conn::m_timer_lst_locker.unlock();
        return;
    }
    EMlog(LOGLEVEL_DEBUG, "===========deleted timer.===========\n");
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
    EMlog(LOGLEVEL_DEBUG, "===========deleted timer.===========\n");
}

// SIGALARM 信号每次触发时在其信号处理函数中执行一次tick()函数，处理到期任务
void sort_timer_lst::tick()
{
    if (!head)
    {
        return;
    }
    EMlog(LOGLEVEL_DEBUG, "timer tick.\n");
    // 从前往后处理,直到遇到一个尚未到期的定时器
    util_timer *tmp = head;
    time_t cur_time = time(NULL);
    while (tmp && tmp->expire<=cur_time)
    {
        // 调用定时器的回调函数，以执行定时任务，关闭连接
        tmp->user_data->close_conn();
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