# 高级IO
非阻塞IO  --  阻塞IO
**补充：有限状态机编程**
## 非阻塞IO
不死等，检测到没有回应，一会儿再检测
- `fcntl()`设置
## IO多路转接
**解决`IO密集型任务`中盲等的问题，监视文件描述符的行为，当当前文件描述符发生了我们感兴趣的行为时，才去做后续操作 可以实现安全的休眠(替代sleep)**
- `select()` 兼容性好 但设计有缺陷 **以事件为单位组织文件描述符**
  - nfds的类型问题
  - 参数没有const修饰 也就是函数会修改 fdset 任务和结果放在一起
  - 监视的事件泰国单一 读 写 异常(异常的种类非常多)
- `poll`
  - wait for some event on a file descriptor （以文件描述符为单位组织事件）
  - `int poll(struct pollfd *fds,nfds,int timeout)`
- `epoll`     **linux的poll封装方案 不可移植**
  - `epoll_create()`，返回一个文件描述符
  - `epoll_ctl(int epfd,int op,int fd,struct epoll_event *event)` 指定epool实例 指定操作动作 指定操作的文件描述符 指定事件  
  ```
   typedef union epoll_data {
     void        *ptr;
     int          fd;
     uint32_t     u32;
     uint64_t     u64;
    } epoll_data_t;
 
    struct epoll_event {
     uint32_t     events;      /* Epoll events */
     epoll_data_t data;        /* User data variable */
    };
  ```
## 其他读写函数
集中读、集中写
- `readv()`
- `writev()` 
## 存储映射IO
**把内存中的内容 或者 某一个文件的内容 映射到当前进程空间中来，可以实现非常快的父子进程的共享内存**
- `mmap(void *addr,size_t length,int prot,int flags,int fd,odd_t offset)`
  - 匿名映射可以实现malloc的功能
- munmap(void *addr,size_t length)，解除映射
## 文件锁
- `fcntl()`
- `lockf()`
- `flock()`