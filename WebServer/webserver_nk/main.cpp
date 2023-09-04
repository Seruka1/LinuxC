#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <assert.h>
#include "threadpool.h"
#include "http_conn.h"

#define MAX_FD 65535           // 最大的文件描述符个数
#define MAX_EVENT_NUMBER 10000 // 最大监听事件数量

static int pipefd[2]; // 创建管道，epoll监听读端0,写端1发送信号

// 添加信号处理函数
void addsig(int sig, void(handler)(int))
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_flags = 0; // 调用sa_handler
    // sigact.sa_flags |= SA_RESTART;         // 指定收到某个信号时是否可以自动恢复函数执行，不需要中断后自己判断EINTR错误信号
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    // sa_mask：信号屏蔽字，用于设置在处理该信号时要屏蔽的信号集。我们不希望在处理信号中断时被其他的信号打断
    sigaction(sig, &sa, NULL);
}

// 向管道写数据的信号捕捉回调函数
void sig_to_pipe(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
    // 写之前保存了errno的值，以便在写操作之后恢复errno的值，防止被信号处理函数修改。
    // SIGALRM 和 SIGTERM 是定义为常量的宏，他们的值分别为14和15
}

// 添加文件描述符到epoll中  listenfd用水平触发lt,其他fd用et
/*et与lt最大区别就是事件通知方式
当用户关心的fd事件发送时，et模式只通知用户一次，不管事件是否已经被用户处理完毕，直到该事件
再次发生，或者用户通过epoll_ctl重新关注fd对应的事件；而lt模式，会不停地通知用户，直到用户把事件处理完毕。
et模式会使得新的就绪事件得到快速的处理    https://zhuanlan.zhihu.com/p/441677252
*/
extern void addfd(int epollfd, int fd, bool one_shot, bool et);

// 删除epoll中的文件描述符
extern void removefd(int epollfd, int fd);

// 修改epoll中的文件描述符
extern void modfd(int epollfd, int fd, int ev);

// 设置文件描述符为非阻塞
extern void set_nonblocking(int fd);

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        EMlog(LOGLEVEL_ERROR, "run as: %s port_number\n", basename(argv[0])); // argv[0] 可能是带路径的，用basename转换
        exit(-1);
    }

    // 获取端口号
    int port = atoi(argv[1]);

    // 对sigpipe信号进行处理
    // 当往一个写端关闭的管道或socket连接中连续写入数据时会引发SIGPIPE信号,引发SIGPIPE信号的写操作将设置errno为EPIPE
    // 因为SIGPIPE信号的默认行为是结束进程，而我们绝对不希望因为写操作的错误而导致程序退出，所以我们捕捉并忽略
    // https://blog.csdn.net/chengcheng1024/article/details/108104507
    addsig(SIGPIPE, SIG_IGN);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0); // 监听套接字

    // 设置端口复用
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 绑定
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    if (ret < 0)
    {
        perror("bind():");
        exit(1);
    }

    // 监听
    ret = listen(listenfd, 5);
    if (ret < 0)
    {
        perror("listen():");
        exit(1);
    }

    // 创建epoll对象，事件数组，添加
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    if (epollfd < 0)
    {
        perror("epoll_create():");
        exit(1);
    }

    // 将监听的文件描述符添加到epoll对象中
    addfd(epollfd, listenfd, false, false);

    // 创建一个数组用于保存所有的客户端信息
    http_conn *users = new http_conn[MAX_FD];
    http_conn::m_epollfd = epollfd;

    // 创建管道  利用epoll来监听信号
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    if (ret < 0)
    {
        perror("socketpair():");
        exit(1);
    }
    set_nonblocking(pipefd[1]);              // 写管道非阻塞
    addfd(epollfd, pipefd[0], false, false); // epoll检测读管道

    // 设置信号处理函数
    addsig(SIGALRM, sig_to_pipe); // 定时信号
    addsig(SIGTERM, sig_to_pipe); // SIGTERM关闭服务器
    bool stop_server = false;

    // 创建线程池
    threadpool<http_conn> *pool = NULL; // 模板类 指定任务类类型为 http_conn
    try
    {
        pool = new threadpool<http_conn>;
    }
    catch (...) //...表示捕获所有类型的异常
    {
        exit(-1);
    }

    bool timeout = false; // 定时器周期已到
    alarm(TIMESLOT);      // 定时产生SIGSLRM信号，间隔为TIMESlOT，epoll监视pipe读端

    while (!stop_server)
    {
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (num < 0 && errno != EINTR)
        {
            // EINTR表示在等待期间收到了中断信号，可以忽略该错误。否则，根据具体情况进行错误处理。
            printf("epoll failure\n");
            break;
        }

        // 循环遍历事件数组
        for (int i = 0; i < num; ++i)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd)
            {
                // 有客户端连接进来
                struct sockaddr_in client_address;
                socklen_t client_addrlen = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlen);

                // ...判断是否连接成功
                if (http_conn::m_user_count >= MAX_FD)
                {
                    // 目前连接数满了
                    // 给客户端写个信息：服务器正忙。
                    close(connfd);
                    continue;
                }
                // 将新客户的数据初始化放入数组当中
                users[connfd].init(connfd, client_address);
                // 当listen_fd也注册了ONESHOT事件时(a**fd)，
                // 接受了新的连接后需要重置socket上EPOLLONESHOT事件，确保下次可读时，EPOLLIN 事件被触发，因此不应该将listen注册为oneshot
                // modfd(epoll_fd, listen_fd, EPOLLIN);
            }
            else if (sockfd == pipefd[0] && (events[i].events & EPOLLIN))
            {
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret < 0)
                {
                    continue;
                }
                if (ret == 0)
                {
                    continue;
                }
                for (int i = 0; i < ret; ++i)
                {
                    switch (signals[i])
                    {
                    case SIGALRM:
                    {
                        // 用timeout变量标记有定时任务需要处理，但不立即处理定时任务
                        // 这是因为定时任务的优先级不是很高，我们优先处理其他更重要的任务
                        timeout = true;
                        break;
                    }
                    case SIGTERM:
                    {
                        stop_server = true;
                        break;
                    }
                    }
                }
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // 客户端异常断开或者错误事件
                EMlog(LOGLEVEL_DEBUG, "-------EPOLLRDHUP | EPOLLHUP | EPOLLERR--------\n");
                users[sockfd].close_conn();
                http_conn::m_timer_lst.del_timer(users[sockfd].m_timer); // 移除其对应的定时器
            }
            else if (events[i].events & EPOLLIN)
            {
                EMlog(LOGLEVEL_DEBUG, "-------EPOLLIN-------\n\n");
                if (users[sockfd].read())
                {
                    // 一次性把所有数据读完
                    pool->append(users + sockfd); // 加入到线程池队列中，数组指针 + 偏移 &users[sock_fd]
                }
                else
                {
                    // 写入失败  关闭连接 移除定时器
                    users[sockfd].close_conn();
                    http_conn::m_timer_lst.del_timer(users[sockfd].m_timer); // 移除其对应的定时器
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                EMlog(LOGLEVEL_DEBUG, "-------EPOLLOUT--------\n\n");
                // 主进程一次性写完所有数据
                if (!users[sockfd].write())
                {
                    // 写入失败，移除其对应的计时器
                    users[sockfd].close_conn();
                    http_conn::m_timer_lst.del_timer(users[sockfd].m_timer); // 移除其对应的定时器
                }
            }
        }
        // 最后处理定时事件，因为I/O事件有更高的优先级。当然，这样做将导致定时任务不能精准地按照预定的时间执行。
        if (timeout)
        {
            // 定时处理任务，实际上就是调用tick()函数
            http_conn::m_timer_lst.tick();
            // setitimer能实现更加精准的定时
            alarm(TIMESLOT);
            timeout = false;
        }
    }
    close(epollfd);
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[0]);
    delete[] users;
    delete pool;

    return 0;
}