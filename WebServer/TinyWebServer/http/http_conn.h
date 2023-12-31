#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>   //文件状态
#include <sys/mman.h>   //内存映射  把磁盘中的数据映射到当前的内存空间当中
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/uio.h>
#include <assert.h>
#include <sys/wait.h>
#include<map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"


//tiny中把定时器也放到一个数组当中，与http用户数组同下标，这样做就不在用户当中再记录一个定时器了
// class sort_timer_lst;
// class util_timer;

// 连接的用户数据类
class http_conn
{
public:                                        // 共享对象，没有线程竞争资源，所以不需要互斥
    static int m_epollfd;                      // 所有socket上的事件都被注册到同一个epoll
    static int m_user_count;                   // 统计用户的数量
    MYSQL* mysql;
    int m_state;                               //0为读，1为写，用于reactor模式

    static const int READ_BUFFER_SIZE = 2048;  // 读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 2048; // 写缓冲区大小
    static const int FILENAME_LEN = 200;       // 文件名的最大长度


public:
    // HTTP请求方法，这里只支持GET/POST  实现注册登录
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT
    };

    /*
        解析客户端请求时，主状态机的状态
        CHECK_STATE_REQUESTLINE:当前正在解析请求行
        CHECK_STATE_HEADER:     当前正在解析头部字段
        CHECK_STATE_CONTENT:    当前正在解析请求体
    */
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    /*
        服务器处理HTTP请求的可能结果，报文解析的结果
        NO_REQUEST          :   请求不完整，需要继续读取客户数据
        GET_REQUEST         :   表示获得了一个完成的客户请求
        BAD_REQUEST         :   表示客户请求语法错误
        NO_RESOURCE         :   表示服务器没有资源
        FORBI**EN_REQUEST   :   表示客户对资源没有足够的访问权限
        FILE_REQUEST        :   文件请求,获取文件成功
        INTERNAL_ERROR      :   表示服务器内部错误
        CLOSED_CONNECTION   :   表示客户端已经关闭连接了
    */
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    // 从状态机的三种可能状态，即行的读取状态，分别表示
    // 1.读取到一个完整的行 2.行出错 3.行数据尚且不完整
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn();
    ~http_conn();

    void process(); // 处理客户端请求
    void init(int sockfd, const sockaddr_in &addr,char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true); // 关闭连接
    bool read_once();       // 非阻塞的读
    bool write();      // 非阻塞的写
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;   //用于同步并发模型指示读写是否出错
    int improv;       //用于同步并发模型指示是否读写完成

private:
    int m_sockfd;                      // 该HTTP连接的socket
    sockaddr_in m_address;             // 通信的socket地址
    char m_read_buf[READ_BUFFER_SIZE]; // 读缓冲区
    int m_read_idx;                    // 标识读缓冲区中已经读入的客户端数据的最后一个字节的下一个位置

    int m_checked_index; // 当前正在分析的字符再读缓冲区的位置
    int m_start_line;    // 当前正在解析的行的起始位置

    char *m_url;                    // 请求目标文件的文件名
    char *m_version;                // 协议版本，只支持HTTP1.1
    METHOD m_method;                // 请求方法
    char *m_host;                   // 主机名
    long m_content_len;             // HTTP请求体的消息总长度
    bool m_linger;                  // HTTP 请求是否要保持连接 keep-alive
    char m_real_file[FILENAME_LEN]; // 客户请求的目标文件的完整路径，其内容等于 doc_root + m_url, doc_root是网站根目录
    CHECK_STATE m_check_state;      // 主状态机当前所处的状态

    struct stat m_file_stat;             // 目标文件的状态。通过它我们可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息
    char *m_file_address;                // 客户请求的目标文件被mmap到内存中的起始位置
    char m_write_buf[WRITE_BUFFER_SIZE]; // 写缓冲区
    int m_write_idx;                     // 写缓冲区中待发送的字节数
    struct iovec m_iv[2];                // writev来执行写操作，表示分散写两个不连续内存块的内容
    /*
        struct iovec {
        void *iov_base;   // 缓冲区的起始地址
        size_t iov_len;   // 缓冲区的长度
        };
        iov_base：表示缓冲区的起始地址，通常是一个指向内存区域的指针。
        iov_len：表示缓冲区的长度，即缓冲区中可用数据的字节数。
        iovec 结构体通常用于 I/O 操作中，如在 readv()、writev() 等函数中，可以通过组合多个 iovec 结构体来一次性读取或写入多个缓冲区的数据，从而提高效率。

    */
    int m_iv_count;      // 被写内存块的数量
    int cgi;             //是否启用POST  也就是是否进行注册登录检验
    char* m_string;      //存储请求体数据
    char* doc_root;      //根目录
    int bytes_to_send;   // 将要发送的字节
    int bytes_have_send; // 已经发送的字节

    map<string,string> m_users;  //记录数据库中的用户密码
    int m_TRIGMode;              //1 ET模式还是  0 为LT模式
    int m_close_log;
    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];          //数据库名

private:
    void init();                       // 私有函数，初始化连接以外的信息
    HTTP_CODE process_read();          // 解析HTTP请求
    bool process_write(HTTP_CODE ret); // 填充HTTP应答

    // 下面这一组函数被process_read调用以分析HTTP请求
    HTTP_CODE parse_request_line(char *text);              // 解析HTTP请求首行
    HTTP_CODE parse_headers(char *text);                   // 解析请求头
    HTTP_CODE parse_content(char *text);                   // 解析请求体
    LINE_STATUS parse_line();                              // 解析具体某一行  从状态机
    char *get_line() { return m_read_buf + m_start_line; } // 获取一行数据
    HTTP_CODE do_request();                                // 处理具体请求

    // 这一组函数被process_write调用以填充HTTP应答。
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_content_type();
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
};

#endif