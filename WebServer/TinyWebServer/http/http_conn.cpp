#include "http_conn.h"

#include <fstream>

// 网站的根目录
// const char *doc_root = "/home/recall123/LinuxC/WebServer/webserver_nk/resources";
// 定义HTTP响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbi**en";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";

int http_conn::m_epollfd = -1; // 类中静态成员需要外部定义
int http_conn::m_user_count = 0;

locker m_lock;
map<string, string> users;

http_conn::http_conn() {}
http_conn::~http_conn() {}

// 每次连接首先把数据库用户数据取到放入map
void http_conn::initmysql_result(connection_pool *connPool)
{
    MYSQL *mysql = NULL;
    connectionRAII mysqlcon(&mysql, connPool);

    // 在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    // 从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    // 返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    // 返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    // 一行一行获取用户名/密码 （游标操作？）,存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string tmp1(row[0]);
        string tmp2(row[1]);
        users[tmp1] = tmp2;
    }
}

// 设置文件描述符非阻塞 返回先前的状态
int set_nonblocking(int fd)
{
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
    return old_flag;
}

// 添加需要监听的文件描述符到epoll中
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;
    if (TRIGMode == 1)
    {
        // EPOLLRDHUP是半关闭连接事件，如果一个连接的对端关闭了写端（即半关闭连接），内核会向应用程序发送EPOLLRDHUP事件通知。
        // 对所有fd设置边沿触发，但是listen_fd不需要，可以另行判断处理
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else
    {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if (one_shot) // 一个socket事件在任意时段最多只有一个线程在处理
    {
        event.events |= EPOLLONESHOT; // 注册为 EPOLLONESHOT事件，防止同一个通信被不同的线程处理
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // 设置文件描述符非阻塞（epoll ET模式）
    set_nonblocking(fd);
}

// 删除epoll中监听的文件描述符
void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// 修改epoll中的文件描述符,重置socket上EPOLLONESHOT事件，以确保下一次可读时EPOLLIN事件能被触发
void modfd(int epollfd, int fd, int ev, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
    {
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    }
    else
    {
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    }

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 初始化连接,外部调用初始化套接字地址
void http_conn::init(int sockfd, const sockaddr_in &addr, char *root, int TRIGMode, int close_log, string user, string passwd, string sqlname)
{
    m_sockfd = sockfd;
    m_address = addr;
    // 端口复用
    // int reuse = 1;
    // setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 添加到epoll对象中,设置oneShot和ET边沿触发
    addfd(m_epollfd, m_sockfd, true, m_TRIGMode);
    m_user_count++;

    // 当浏览器出现连接重置时，可能是网站根目录出错或http响应格式出错或者访问的文件中内容完全为空
    doc_root = root;
    m_TRIGMode = TRIGMode;
    m_close_log = close_log;

    strcpy(sql_user, user.c_str());
    strcpy(sql_passwd, passwd.c_str());
    strcpy(sql_name, sqlname.c_str());
    init();
}

// 初始化新接受的连接
// check_state默认为分析请求行状态
void http_conn::init()
{
    mysql = NULL;
    m_check_state = CHECK_STATE_REQUESTLINE; // 初始化状态为正在解析请求首行
    m_checked_index = 0;                     // 初始化解析字符索引
    m_start_line = 0;                        // 行的起始位置
    m_read_idx = 0;                          // 读取字符的位置

    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_linger = false;  // 默认不保持连接
    m_content_len = 0; // HTTP请求体的消息总长度
    m_host = 0;

    m_write_idx = 0;
    bytes_have_send = 0;
    bytes_to_send = 0;

    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    bzero(m_read_buf, READ_BUFFER_SIZE);   // 清空读缓存
    bzero(m_write_buf, WRITE_BUFFER_SIZE); // 清空写缓存
    bzero(m_real_file, FILENAME_LEN);      // 清空文件路径
}

// 关闭连接
void http_conn::close_conn(bool real_close)
{
    if (real_close && m_sockfd != -1)
    {
        printf("close %d\n", m_sockfd);
        m_user_count--;
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
    }
}

// 循环读取客户数据，直到无数据可读或者对方关闭连接
// 非阻塞ET工作模式下，需要一次性将数据读完，因为eoll只通知一次
bool http_conn::read_once()
{

    if (m_read_idx >= READ_BUFFER_SIZE)
    {
        // 超过缓冲区大小
        return false;
    }

    // 读取到的字节
    int bytes_read = 0;

    // LT读取数据  没读完会再次通知的
    if (0 == m_TRIGMode)
    {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += bytes_read;
        if (bytes_read <= 0)
        {
            return false;
        }
        return true;
    }
    else
    {
        while (true)
        {
            // m_sock_fd已设置非阻塞，因此recv函数的第四个参数flags不用再设置为MSG_DONTWAIT表示非阻塞，设置为0即可
            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            if (bytes_read == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // 非阻塞读取，没有数据了，recv函数非阻塞返回-1时，并且错误码是 EAGAIN 或 EWOULDBLOCK，表示缓冲区中没有数据可读，可以继续尝试或者退出
                    break;
                }
                return false;
            }
            else if (bytes_read == 0)
            {
                // 对方关闭连接
                // 在使用套接字进行网络通信时，当对方关闭了连接，recv 函数会返回 0，表示已经没有数据可读了。
                // 这是因为在 TCP 协议中，对方关闭连接会发送一个 FIN 信号，表示不再发送数据，这时本地套接字收到 FIN 信号后，recv 函数会返回 0，表示对方已经关闭了连接。
                return false;
            }
            m_read_idx += bytes_read;
        }
        return true;
    }
}

// 主状态机：解析请求
http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_state = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;

    // 解析到了头部/请求行一行完整的数据，或者解析到了请求体一行完整的数据（特判请求体是因为其结尾不一样 不是\r\n）
    while ((m_check_state == CHECK_STATE_CONTENT && line_state == LINE_OK) || ((line_state = parse_line()) == LINE_OK))
    {
        // 解析到了一行完整的数据/请求体
        // char* get_line(){return m_rd_buf + m_line_start;}前面的parse_one_line给我们设置了'\0'，或者头部和请求行给我们设置好了换行
        text = get_line();

        // 更新下一行的起始位置
        m_start_line = m_checked_index;

        LOG_INFO("%s", text);
        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER:
        {
            ret = parse_headers(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if (ret == GET_REQUEST)
            {
                return do_request(); // 解析具体的请求信息
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if (ret == GET_REQUEST)
            {
                return do_request();
            }
            line_state = LINE_OPEN; // 请求体还没读完，说明数据没接收完全，利用LINE_OPEN跳出循环
            break;
        }
        default:
        {
            return INTERNAL_ERROR; // 内部错误
        }
        }
    }

    return NO_REQUEST; // 数据不完整
}

// 处理具体请求
// 当得到一个完整、正确的HTTP请求时，我们就分析目标文件的属性，
// 如果目标文件存在、对所有用户可读，且不是目录，则使用mmap将其
// 映射到内存地址m_file_address处，并告诉调用者获取文件成功

http_conn::HTTP_CODE http_conn::do_request()
{
    // "/home/cyf/Linux/webserver/resources"
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    // 根据m_url /后的第一个字符判断请求资源
    const char *p = strrchr(m_url, '/');
    // 处理cgi  注册登录验证
    if (cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3'))
    {
        // 这里是不是有问题？
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/");
        strcat(m_url_real, m_url + 2);
        strncpy(m_real_file + len, m_url_real, FILENAME_LEN - len - 1);
        free(m_url_real);

        // 将用户名和密码提取出来
        // user=123&passwd=123
        char name[100], password[100];
        int i;
        for (i = 5; m_string[i] != '&'; ++i)
        {
            name[i - 5] = m_string[i];
        }
        name[i - 5] = '\0';
        int j = 0;
        for (i = i + 10; m_string[i] != '\0'; ++i, ++j)
        {
            password[j] = m_string[i];
        }
        password[j] = '\0';

        // 第一个字符为2表示登录，3表示注册
        if (*(p + 1) == '3')
        {
            // 注册的话  如果成功就需要往数据库中添加数据
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");
            if (users.find(name) == users.end())
            {
                // 更新数据库
                m_lock.lock();
                int res = mysql_query(mysql, sql_insert);
                users.insert(pair<string, string>(name, password));
                m_lock.unlock();

                if (!res)
                {
                    // 注册成功
                    strcpy(m_url, "/log.html");
                }
                else
                {
                    // 注册失败
                    strcpy(m_url, "/registerError.html");
                }
            }
            else
            {
                // 该用户已经注册过了
                strcpy(m_url, "/registerError.html");
            }
        }
        else //*(p + 1) == '2'   登录，直接判断map中有没有数据
        {
            if (users.find(name) != users.end() && users[name] == password)
            {
                strcpy(m_url, "/welcome.html");
            }
            else
            {
                strcpy(m_url, "/logError.html");
            }
        }
    }

    if (*(p + 1) == '0')
    {
        // 获取注册页面
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/register.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));
        free(m_url_real);
    }
    else if (*(p + 1) == '1')
    {
        // 获取登录页面
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/log.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '5')
    {
        // 获取图片
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/picture.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '6')
    {
        // 获取视频
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/video.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '7')
    {
        // 获取点赞页面
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/fans.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else
    {
        strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
    }

    // 获取m_real_file文件的相关的状态信息，-1失败，0成功
    if (stat(m_real_file, &m_file_stat) < 0)
    {
        // int stat(const char *path, struct stat *buf);其中，path 是要获取状态的文件或目录的路径，buf 是用于保存状态信息的结构体指针。buf是传出参数
        return NO_RESOURCE;
    }

    // 判断访问权限
    if (!(m_file_stat.st_mode & S_IROTH))
    { // S_IROTH 是一个用于表示其他用户（除了文件所有者和文件所属组）对文件的读权限的宏定义
        return FORBIDDEN_REQUEST;
    }

    // 判断是否是目录
    if (S_ISDIR(m_file_stat.st_mode))
    { // S_ISDIR 宏定义的值是一个函数，用于检查指定的文件模式是否表示一个目录。
        // 如果文件模式表示一个目录，则 S_ISDIR 返回非零值（真），否则返回零值（假）
        return BAD_REQUEST;
    }

    // 以只读方式打开文件
    int fd = open(m_real_file, O_RDONLY);
    // 创建内存映射,m_file_a**ress是客户请求的目标文件被mmap到内存中的起始位置
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    // a**r：指定映射的起始地址，通常设置为 NULL，表示由系统自动选择合适的地址，0和null等效
    // void *mmap(void *a**r, size_t length, int prot, int flags, int fd, off_t offset);
    // MAP_PRIVATE：映射区域与文件不共享，对映射区域的修改不会影响到文件
    // 只读打开，PROT也是读

    close(fd);
    return FILE_REQUEST;
}

// 对内存映射区执行munmap操作
void http_conn::unmap()
{
    if (m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size);
        // int munmap(void* a**r, size_t length);第一个参数是映射区域的起始地址
        // 第二个参数是映射区域的长度，与mmap的第二个参数需要保持一致

        m_file_address = 0;
    }
}

// 解析请求行，获取请求方法，目标URL，HTTP版本
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    // GET /index.html HTTP/1/1
    m_url = strpbrk(text, " \t");
    if (!m_url)
    {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
    {
        m_method = GET;
    }
    else if (strcasecmp(method, "POST") == 0)
    {
        cgi = 1;
        m_method = POST;
    }
    else
    {
        return BAD_REQUEST;
    }
    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if (!m_version)
    {
        return BAD_REQUEST;
    }
    *m_version++ = '\0'; // /index.html\0HTTP/1.1，此时m_url到\0结束，表示 /index.html\0      // HTTP/1.1
    m_version += strspn(m_version, " \t");

    // 这里注释的话，接收的信息太多了 qps下降
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }

    // 可能出现带地址的格式 http://192.168.15.128.1:9999/index.html
    if (strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;                 // 192.168.15.128.1:9999/index.html
        m_url = strchr(m_url, '/'); // /index.html
    }
    else if (strncasecmp(m_url, "https://", 8) == 0)
    {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || m_url[0] != '/')
    {
        return BAD_REQUEST;
    }
    // 当url为/时，显示判断页面
    if (strlen(m_url) == 1)
    {
        strcat(m_url, "judge.html");
    }
    m_check_state = CHECK_STATE_HEADER; // 主状态机状态改变为检查请求头部
    return NO_REQUEST;                  // 请求尚未解析完成
}
http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    // 遇到空行，表示头部字段解析完毕
    if (text[0] == '\0')
    {
        // 如果HTTP请求有消息体，则还需要读取m_content_length字节的消息体，
        // 状态机转移到CHECK_STATE_CONTENT状态
        if (m_content_len != 0)
        { // 请求体有内容
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        // 否则说明HTTP请求没有请求体，读完请求行后我们已经得到了一个完整的HTTP请求
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        // 处理Connection 头部字段  Connection: keep-alive
        text += 11;
        text += strspn(text, " \t"); // 检索字符串 str1 中第一个不在字符串 str2 中出现的字符下标。
        // size_t strspn(const char* str1,const char* str2)
        // 返回值是str1连续包含str2中字符的个数
        if (strcasecmp(text, "keep-alive") == 0)
        {
            m_linger = true;
        }
    }
    else if (strncasecmp(text, "Content-Length:", 15) == 0)
    {
        // 处理Content-Length头部字段
        text += 15;
        text += strspn(text, " \t");
        m_content_len = atol(text);
        /*
        * long int atol(const char *nptr);
          atol 函数会从字符串 nptr 开始解析整数，直到遇到非数字字符为止，然后将解析到的整数值作为结果返回。如果字符串中没有有效的整数，atol 函数将返回 0。
        */
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        // 处理Host头部字段
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else
    {
        LOG_INFO("oop! unknow header: %s\n", text);
    }
    return NO_REQUEST;
}

// 解析请求体 判断它是否被完整读入
http_conn::HTTP_CODE http_conn::parse_content(char *text)
{
    if (m_read_idx >= (m_content_len + m_checked_index)) // 读到的数据长度 大于 已解析长度（请求行+头部+空行）+请求体长度
    {                                                    // 数据被完整读取
        text[m_content_len] = '\0';                      // 标志结束
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// 解析一行
http_conn::LINE_STATUS http_conn::parse_line()
{
    char tmp;
    for (; m_checked_index < m_read_idx; ++m_checked_index)
    {
        tmp = m_read_buf[m_checked_index];

        if (tmp == '\r')
        {
            if (m_checked_index == m_read_idx - 1) // 回车符是已经读到的最后一个字符，表示行数据尚不完整
            {
                return LINE_OPEN;
            }
            else if (m_read_buf[m_checked_index + 1] == '\n')
            {
                m_read_buf[m_checked_index++] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (tmp == '\n')
        {
            if (m_checked_index > 1 && m_read_buf[m_checked_index - 1] == 'r')
            {
                m_read_buf[m_checked_index - 1] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN; // 没到结束符，数据不完整
}

// 写HTTP响应数据
bool http_conn::write()
{
    int temp = 0;

    if (bytes_to_send == 0)
    {
        // 将要发送的字节为0，这一次响应结束。重新入队，设置oneshot
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        init();
        return true;
    }
    while (1)
    {
        // 分散写   m_write_buf + m_file_a**ress
        temp = writev(m_sockfd, m_iv, m_iv_count);
        /*
            ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
            fd 是文件描述符，iov 是一个指向 iovec 结构体数组的指针，iovcnt 是 iov 数组中结构体的数量。
            writev() 函数将 iov 数组中的多个缓冲区的数据一并写入到文件描述符 fd 指向的文件中，实现了多个缓冲区的批量写入操作。返回值为写入的字节数，
            如果执行失败，返回值为 -1，且 errno 的值为 EAGAIN 或 EWOULDBLOCK，通常表示 TCP 写缓冲区没有空间，即发送缓冲区已满。
        */
        if (temp <= -1)
        {
            // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件，将其modfd重新加入epoll队列。虽然在此期间，
            // 服务器无法立即接收到同一客户的下一个请求，但可以保证连接的完整性。
            if (errno == EAGAIN)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
                return true;
            }
            unmap(); // 释放内存映射m_file_a**ress空间
            return false;
        }

        bytes_to_send -= temp;
        bytes_have_send += temp;

        if (bytes_have_send >= m_iv[0].iov_len)
        {                                                                        // 发完头部了
            m_iv[0].iov_len = 0;                                                 // 更新两个发送内存块的信息
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx); // 已经发了部分的响应体数据
            m_iv[1].iov_len = bytes_to_send;
        }
        else
        { // 还没发完头部，更新头部m_iv[0]中下一轮发送的起始下标和长度
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - temp;
        }

        if (bytes_to_send <= 0)
        {
            // 没有数据要发送了
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode); // 重新加入epoll队列，设置读事件监听

            if (m_linger)
            { // 如果m_linger为true，保持连接，我们重新初始化
                init();
                return true;
            }
            else
            {
                return false; // 返回值为false时，main函数中就帮我们销毁了
            }
        }
    }
}

// 往写缓冲中写入待发送的数据
bool http_conn::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
    { // 写缓冲区满了
        return false;
    }
    va_list arg_list;           // 可变参数，格式化文本
    va_start(arg_list, format); // 添加文本到到写缓冲区m_write_buf中
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        va_end(arg_list);
        return false; // 没写完，已经满了
    }
    m_write_idx += len; // 更新下次写数据的起始位置
    va_end(arg_list);

    // 这里日志记录有没有问题？
    LOG_INFO("request:%s", m_write_buf);
    return true;
    /*
        va_list是一个类型，用于存储传递给可变参数函数的参数列表

        va_start()函数需要两个参数，第一个参数是我们定义的va_list类型的变量，第二个参数是可变参数列表中最后一个已知的参数

        va_end()传入start中传入的va_list类型的变量

        int vsnprintf(char* str,size_t len,const char* format,va_list ap)
            str是一个指向字符数组的指针，用于存储生成的字符串
            size是字符数组的大小，限制生成的字符串的最大长度
            format是一个格式化字符串，用于指定生成字符串的格式
            ap是一个va_list类型的可变参数列表，用于填充格式化字符串中的控制符
    */
}

// 添加状态码（响应行）
bool http_conn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

// 添加了一些必要的响应头部
bool http_conn::add_headers(int content_len)
{
    return add_content_length(content_len) && add_content_type() && add_blank_line();
}

bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length: %d\r\n", content_len);
}
bool http_conn::add_content_type()
{ // 响应体类型，当前文本形式，这里我们写死了类型是text/html
    return add_response("Content-Type:%s\r\n", "text/html");
}
bool http_conn::add_linger()
{
    return add_response("Connection: %s\r\n", (m_linger == true) ? "keep-alive" : "close");
}
bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

bool http_conn::add_content(const char *content)
{
    return add_response("%s", content);
}
// 根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
        {
            return false;
        }
        break;
    case BAD_REQUEST:
        add_status_line(400, error_400_title);
        add_headers(strlen(error_400_form));
        if (!add_content(error_400_form))
        {
            return false;
        }
        break;
    case NO_RESOURCE:
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
        {
            return false;
        }
        break;
    case FORBIDDEN_REQUEST:
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
        {
            return false;
        }
        break;
    case FILE_REQUEST: // 请求文件
        add_status_line(200, ok_200_title);
        if (m_file_stat.st_size != 0)
        {
            add_headers(m_file_stat.st_size);
            // 封装m_iv
            m_iv[0].iov_base = m_write_buf; // 起始地址
            m_iv[0].iov_len = m_write_idx;  // 长度
            m_iv[1].iov_base = m_file_address;
            m_iv[1].iov_len = m_file_stat.st_size;
            m_iv_count = 2;                                    // 两块内存
            bytes_to_send = m_write_idx + m_file_stat.st_size; // 响应头的大小 + 文件的大小
            return true;
        }
        else
        {
            // 当文件为空的时候返回一个空界面
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
            {
                return false;
            }
        }
    default:
        return false;
    }

    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

// 由线程池中工作线程调用的
void http_conn::process()
{
    // 解析HTTP请求
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        return;
    }

    // 生成响应
    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn();
        return;
    }

    modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode); // 重置EPOLLONESHOT
}