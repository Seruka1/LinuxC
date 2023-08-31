#include"http_conn.h"

int http_conn::m_epollfd=-1;
int http_conn::m_user_count=0;

//设置文件描述符非阻塞
void setnonblocking(int fd){
    int old_flag=fcntl(fd,F_GETFL);
    int new_flag=old_flag|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_flag);
}

//添加文件描述符到epoll中
void addfd(int epollfd,int fd,bool one_shot)
{
    epoll_event event;
    event.data.fd=fd;
    // event.events=EPOLLIN|EPOLLRDHUP;
    event.events=EPOLLIN|EPOLLET|EPOLLRDHUP;
    if(one_shot)   //一个socket事件在任意时段最多只有一个线程在处理
    {
        event.events|=EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    //设置文件描述符非阻塞
    setnonblocking(fd);
}

//删除epoll中监听的文件描述符
void removefd(int epollfd,int fd)
{
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

//修改epoll中的文件描述符,重置socket上EPOLLONESHOT事件，以确保下一次可读时EPOLLIN事件能被触发
void modfd(int epollfd,int fd,int ev)
{
    epoll_event event;
    event.data.fd=fd;
    event.events=ev|EPOLLONESHOT|EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

//初始化连接
void http_conn::init(int sockfd,const sockaddr_in &addr)
{
    m_sockfd=sockfd;
    m_address=addr;
    //端口复用
    int reuse=1;
    setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    //添加到epoll对象中
    addfd(m_epollfd,m_sockfd,true);
    m_user_count++;    

    init();
}

//初始化连接之外的信息
void http_conn::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE; // 初始化状态为正在解析请求首行
    m_checked_index = 0;                      // 初始化解析字符索引
    m_start_line=0;
    m_read_idx=0;
    m_method=GET;
    m_url=0;
    m_version=0;
    m_linger=false;
    bzero(m_read_buf, READ_BUFFER_SIZE);           // 清空读缓存
}

//关闭连接
void http_conn::close_conn()
{
    if(m_sockfd!=-1)
    {
        removefd(m_epollfd,m_sockfd);
        m_sockfd=-1;
        m_user_count--;   
    }
}


//循环读取客户数据，直到无数据可读或者对方关闭连接
bool http_conn::read()
{
    if(m_read_idx>=READ_BUFFER_SIZE)
    {
        return false;
    }

    //读取到的字节
    int bytes_read=0;
    while(true)
    {
        bytes_read=recv(m_sockfd,m_read_buf+m_read_idx,READ_BUFFER_SIZE-m_read_idx,0);
        if(bytes_read==-1)
        {
            if(errno==EAGAIN||errno==EWOULDBLOCK)
            {
                //没有数据
                break;
            }
            return false;
        }
        else if(bytes_read==0)
        {
            //对方关闭连接
            return false;
        }
        m_read_idx+=bytes_read;
    }   
    printf("读取到了数据：%s\n",m_read_buf);
    return true;
}


//主状态机：解析请求
http_conn::HTTP_CODE  http_conn::process_read()
{
    LINE_STATUS line_state=LINE_OK;
    HTTP_CODE ret=NO_REQUEST;
    char *text=0;
    while((m_check_state==CHECK_STATE_CONTENT&&line_state==LINE_OK)||((line_state=parse_line())==LINE_OK))
    {
        //解析到了一行完整的数据/请求体
        text=get_lint();
        m_start_line=m_checked_index;
        printf("got 1 http line:%s\n",text);

        switch(m_check_state)
        {
            case CHECK_STATE_REQUESTLINE:
            {
                ret=parse_request_line(text);
                if(ret==BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                ret=parse_headers(text);
                if(ret==BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                else if(ret==GET_REQUEST)
                {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                ret=parse_content(text);
                if(ret==GET_REQUEST)
                {
                    return do_request();
                }
                line_state=LINE_OPEN;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }

    return NO_REQUEST;
}

// 处理具体请求
http_conn::HTTP_CODE http_conn::do_request()
{

}                        

//解析请求行，获取请求方法，目标URL，HTTP版本
http_conn::HTTP_CODE  http_conn::parse_request_line(char* text)
{
    //GET /index.html HTTP/1/1
    m_url=strpbrk(text," \t");
    *m_url++='\0';
    char* method=text;
    if(strcasecmp(method,"GET")==0)
    {
        m_method=GET;
    }
    else 
    {
        return BAD_REQUEST;
    } 
    m_version = strpbrk(m_url, " \t");
    if(!m_version) 
    {
        return BAD_REQUEST;
    }
    *m_version = '\0';  // /index.html\0HTTP/1.1，此时m_url到\0结束，表示 /index.html\0
    m_version++;        // HTTP/1.1
    if(strcasecmp(m_version,"HTTP/1.1")!=0)
    {
        return BAD_REQUEST;
    }

     // 可能出现带地址的格式 http://192.168.15.128.1:9999/index.html
    if(strncasecmp(m_url,"http://",7)==0)
    {
        m_url+=7;// 192.168.15.128.1:9999/index.html
        m_url=strchr(m_url,'/');// /index.html
    }
    if(!m_url || m_url[0] != '/')
    {
        return BAD_REQUEST;
    }

    m_check_state = CHECK_STATE_HEADER;  // 主状态机状态改变为检查请求头部
    return NO_REQUEST;                  // 请求尚未解析完成

}
http_conn::HTTP_CODE  http_conn::parse_headers(char* text)
{
    
}
http_conn::HTTP_CODE  http_conn::parse_content(char* text)
{

}

//解析一行
http_conn::LINE_STATUS http_conn::parse_line()
{
    char tmp;
    for(;m_checked_index<m_read_idx;++m_checked_index)
    {
        tmp=m_read_buf[m_checked_index];

        if(tmp=='\r')
        {
            if(m_checked_index==m_read_idx-1)  // 回车符是已经读到的最后一个字符，表示行数据尚不完整
            {
                return LINE_OPEN;
            }
            else if(m_read_buf[m_checked_index+1]=='\n')
            {
                m_read_buf[m_checked_index++]='\0';
                m_read_buf[m_checked_index++]='\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(tmp=='\n')
        {
            if(m_checked_index>1&&m_read_buf[m_checked_index-1]=='r')
            {
                m_read_buf[m_checked_index-1]='\0';
                m_read_buf[m_checked_index++]='\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }

    }
    return LINE_OPEN;  //没到结束符，数据不完整
}

bool http_conn::write()
{
    printf("一次性写完数据\n");
    return true;
}

//由线程池中工作线程调用的
void http_conn::process()
{
    //解析HTTP请求
    HTTP_CODE read_ret=process_read();
    if(read_ret==NO_REQUEST)
    {
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        return;
    }

    printf("parse request,create response\n");
    //生成响应
}