# 单机IPC（进程间通信）
IPC Inter-Process Communication
## 管道
- **由linux内核创建与维护**
- **单工 一端读 一端写**
- **自同步机制 流速快的 等 流速慢的**
- #### 匿名管道
**用于有血缘关系的进程间通信**
- pipe()    
  
形同 `cat ./test.mp3 | mpg123 -`
#### 命名管道
- mkfifo 
~~~ bash
makefifo musicpipe
cat ./test.mp3 > musicpipe
~~~

~~~ bash
#同一个目录
mpg123 musicpipe
~~~

## XSI(SysV)
**共享内存、消息队列、信号量数组**  
主动端：先发包的一方
被动端：先收包的一方（先运行）  
有血缘关系的进程：不需要获取key  
无血缘关系的进程：需要提供path获取唯一的key值
- `ipcs`命令查看IPC
- `ipcrm`删除IPC
- ftok() **使得通信双方拿到同一个机制**
- 函数的命名方式类似
    - xxxget 获取
    - xxxop 操作
    - xxxctl 控制
### 消息队列 Message Queues
- `msgget()`
- `msgop()`
- `msgctl()`
### 信号量数组 Semaphore Arrays
**多个资源的请求**
- `semget()`
- `semop()`
- `semctl()`
### 共享内存 Shared Memory
- `shmget()`
- `shmop()`
- `shmctl()`
- `shmdt()`解除映射
  