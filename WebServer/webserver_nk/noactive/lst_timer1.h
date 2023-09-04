#ifndef LST_TIMER_H
#define LST_TIMER_H

#include<stdio.h>
#include<time.h>
#include <arpa/inet.h>
#include"../http_conn.h"
#include"../locker.h"
#include"../log.h"

class http_conn;
// 定时器类
class util_timer
{
public:
    util_timer();

public:
    time_t expire;          //任务超时时间，这里使用绝对时间
    http_conn *user_data;   //任务数据
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

#endif