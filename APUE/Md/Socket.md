# 跨主机IPC(进程间通信)

## 网络套接字(socket)

## 跨主机的传输要注意的问题

### 字节序

- 大端 **低地址放高字节**
- 小端 **高地址放低字节（x86）**

```c
0x00 00 00 05

//大端
05 00 00 00
//小端
00 00 00 05
```

- 主机字节序 **host**
- 网络字节序 **network**
- _ to _ 长度()
  - htons()
  - htonl()
  - ntohs()
  - ntohl()

### 对齐

```c
struct{
    int i;
    char ch;
    float f;
};//12个字节
```

结构体内存对齐

- 解决方案 **指定宏 告诉编译器不对齐** 结构体后加 `__attribute__((packed));`

### 类型长度

- int的长度未定义
- char有无符号未定义
  解决： `int32_t` `uint32_t` `int64_t` `int8_t(有符号的char)` `uint8_t(无符号的char)`

### socket

**一个中间层，连接网络协议与文件操作**
socket就是插座，在计算机中两个进程通过socket建立起一个通道，数据在通道中传输
socket把复杂的TCP/IP协议族隐藏了起来，对于程序猿来说只要用好socket相关的函数接可以完成网络通信

socket提供了 `stream` `datagram` 两种通信机制，即流socket和数据包socket

- 流socket基于TCP协议，是一个有序、可靠、双向字节流的通道，传输数据不会丢失、不会重复、顺序也不会错乱
- 数据报socket基于UDP协议，不需要建立可靠连接，可能会丢失或错乱。UDP不是一个可靠的协议，对数据的长度有限制，但是效率较高。

```c
int socket(int domain,int type,int protocol)
          //SOCK_STREAM 有序 可靠 双工 基于(单字节)字节流
          //SOCK_DGRAM 分组的 不可靠的 无连接
          //SOCK_SEQPACKET (有序分组式) 有序可靠 报式传输
```

#### 常用函数

绑定地址和端口

- int bind(int socket, const struct sockaddr \*address, socklen_t address_len)
  sockaddr -> struct socketaddr_in

```c
 struct sockaddr_in {
               sa_family_t    sin_family; /* address family: AF_INET */
               in_port_t      sin_port;   /* port in network byte order */
               struct in_addr sin_addr;   /* internet address */
           };

```

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

### 多点通讯

#### 广播(全网广播，子网广播)

- getsockopt()
- setsockopt()

#### 多播/组播

相较广播更灵活
`224.0.0.1` **这个地址表示所有支持多播的节点默认都存在于这个组中且无法离开**  
ip ad sh看网络索引号

#### UDP需要特别注意的问题

**丢包是由阻塞造成的(网络被路由器或其他网络节点按照某种算法移除),而不是ttl，需要进行流量控制**

- TTL time to live  数据包跳转的路由数

Client&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Server
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;------文件路径--->
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;文件
`<br>`
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<-----data1--------
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<-----data2--------
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<-----data3--------
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<------EOT---------
文件

由于网络的不可靠，如果出现丢包将会使得文件传输不完整，如何确保对方一定能收到完整内容呢？
**停等式流控**

Client&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Server
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;--文件路径-->
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;文件
`<br>`
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<-----data1--------等待
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-------ACK1------>继续发送
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<-----data2--------等待
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-------ACK2------>继续发送
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<-----data3--------等待
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-------ACK3------>继续发送
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<------EOT---------等待
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;--------ACK4----->停止发送
文件

中途如果有ACK包Server没有成功收到那么将会再次发送数据包，来回通信延迟（Round-trip delay time），在通信（Communication）、电脑网络（Computer network）领域中，意指：在双方通信中，发讯方的信号（Signal）传播（Propagation）到收讯方的时间（意即：传播延迟（Propagation delay）），加上收讯方回传消息到发讯方的时间（如果没有造成双向传播速率差异的因素，此时间与发讯方将信号传播到收讯方的时间一样久）

## TCP

- TCP三次握手，半连接洪水攻击，syn cookie方法解决半连接洪水攻击。
- netstat -ant（t指tcp）看Socket
- nc/telnet ip port 自带的客户端 

**关于TCP更多细节请看**
[TCP 的那些事儿（上）](https://coolshell.cn/articles/11564.html)
[TCP 的那些事儿（下）](https://coolshell.cn/articles/11609.html)
### 步骤
**Client**
1. 获取SOCKET
2. 给SOCKET取得地址
3. 发送连接
4. 收/发消息 **注意可以以流的形式打开socket描述符进行操作**
5. 关闭  
**Server**
1. 获取SOCKET
2. 给SOCKET取得地址
3. 将SOCKET置为监听模式
4. 接受连接
5. 收/发消息
6. 关闭
