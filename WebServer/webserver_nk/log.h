#ifndef _EM_LOG_H_
#define _EM_LOG_H_

#include<stdarg.h>
#include<stdio.h>
#include"locker.h"
#include"http_conn.h"
#include"lst_timer.h"

//#define OPEN_LOG 1                 //声明是否打开日志输出
#define LOG_LEVEL LOGLEVEL_INFO   // 声明当前程序的日志等级状态，只输出等级等于或高于该值的内容
#define LOG_SAVE 0                 // 可补充日志保存功能

typedef enum
{
    LOGLEVEL_DEBUG = 0,
    LOGLEVEL_INFO,
    LOGLEVEL_WARN,
    LOGLEVEL_ERROR
} E_LOGLEVEL;

void EM_log(const int level, const char *fun, const int line, const char *fmt, ...);

#define EMlog(level, fmt...) EM_log(level, __FUNCTION__, __LINE__, fmt) // 宏定义，隐藏形参

/*
    宏定义的语法格式是 #define 宏名称 替代文本
    其中宏名称是定义的宏的名称，替代文本是宏在调用时会被替代的内容。
    EMlog 是宏的名称，level 和 fmt... 是宏的参数，在调用时需要传递相应的参数值。
    _FUNCTION__ 是C语言的内置宏，表示当前函数的名称，__LINE__ 也是C语言的内置宏，表示当前代码行号。fmt... 表示宏接受可变参数。
    使用 EMlog 宏进行日志输出时，宏会自动将当前的日志等级、函数名、行号等作为参数传递给 EM_log 函数，从而实现了简化日志输出的目的。
*/

/*
    __FUNCTION__：它是一个字符串常量，表示当前代码所在的函数的名称。在使用时，__FUNCTION__ 会被替换成当前函数的名称
    __LINE__ 宏会被替换为当前代码所在的行号，编译器会在编译时根据代码的实际行号来替换它的值
    需要注意的是，行号的计数从1开始，而不是从0开始。
*/
/*
    这两个相当于参数有默认初始值，我们不用在传入的时候传 __FUNCTION__，和__LINE__
    调用EMlog(LOGLEVEL_ERROR,"EPOLL failed.\n")和EMlog(LOGLEVEL_ERROR,__FUNCTION__, __LINE__,"EPOLL failed.\n")是一样的
    输出：[ERROR]     [函数名 函数所在行号]: EPOLL failed.

*/
#endif