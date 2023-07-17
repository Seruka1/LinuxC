# 跨主机IPC(进程间通信)

## 网络套接字(socket)

## 跨主机的传输要注意的问题
### 字节序
- 大端 **低地址放高字节**
- 小端 **高地址放低字节（x86）**
~~~ c
0x00 00 00 05

//大端
05 00 00 00
//小端
00 00 00 05
~~~
- 主机字节序 **host**
- 网络字节序 **network**
- _ to _ 长度()
    - htons()
    - htonl()
    - ntohs()
    - ntohl()
### 对齐
~~~ c
struct{
    int i;
    char ch;
    float f;
};//12个字节
~~~
结构体内存对齐
- 解决方案 **指定宏 告诉编译器不对齐** 结构体后加`__attribute__((packed));`
### 类型长度
- int的长度未定义
- char有无符号未定义
解决： `int32_t` `uint32_t` `int64_t` `int8_t(有符号的char)` `uint8_t(无符号的char)`

### socket
**一个中间层，连接网络协议与文件操作**
socket就是插座，在计算机中两个进程通过socket建立起一个通道，数据在通道中传输
socket把复杂的TCP/IP协议族隐藏了起来，对于程序猿来说只要用好socket相关的函数接可以完成网络通信

socket提供了`stream` `datagram` 两种通信机制，即流socket和数据包socket

- 流socket基于TCP协议，是一个有序、可靠、双向字节流的通道，传输数据不会丢失、不会重复、顺序也不会错乱
- 数据报socket基于UDP协议，不需要建立可靠连接，可能会丢失或错乱。UDP不是一个可靠的协议，对数据的长度有限制，但是效率较高。
~~~ c
int socket(int domain,int type,int protocol)
          //SOCK_STREAM 有序 可靠 双工 基于(单字节)字节流
          //SOCK_DGRAM 分组的 不可靠的 无连接
          //SOCK_SEQPACKET (有序分组式) 有序可靠 报式传输
~~~

#### 常用函数
绑定地址和端口
- int bind(int socket, const struct sockaddr \*address, socklen_t address_len)
sockaddr -> struct socketaddr_in
~~~ c
 struct sockaddr_in {
               sa_family_t    sin_family; /* address family: AF_INET */
               in_port_t      sin_port;   /* port in network byte order */
               struct in_addr sin_addr;   /* internet address */
           };

~~~

- recvfrom()
- sendto()

- inet_pton() 点分式转二进制数

## UDP
### 步骤
- 被动端
    - 取得SOCKET
    - 给SOCKET取得地址
    - 收/发消息
    - 关闭SOCKET
- 主动端
    - 取得SOCKET
    - 给SOCKET取得地址(可以省略 本地通信地址由操作系统分配)
    - 发/收消息
    - 关闭SOCKET

- `socket()`
- `bind()`
- `sendto()`
- `rcvfrom()`
- `inet_pton()`
- `inet_ntop()`
