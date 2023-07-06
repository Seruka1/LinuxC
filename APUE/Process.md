# 进程基本知识
## 1.PID
类型 pid_t  
命令ps
- ps axf 查看进程树
- ps axm
- ps ax -L
- ps -ef  
- ps axj 看守护进程
**进程号是顺次向下使用的，与文件描述符ID有所区别**  
- `getpid()`
- `getppid()`

## 2.进程的产生
- `fork()`
  - 注意理解关键字：duplicate，意味着拷贝，克隆
  - fork 后父子进程的区别 ： fork 的返回值不一样 pid不同 ppid也不同 未决信号与文件🔓 不继承，资源利用量清0 
  - init进程 是所以进程的祖先进程 pid == 1 
  - 调度器的调度策略来决定哪个进程先运行
  - `fflush()`的重要性，fork时缓冲区也会复制
  - `vfork()`
## 3.进程的消亡以及释放资源
- `wait()`
- `waitpid()`
- `waitid()`
- `wait3()`
- `wait4()`
## 4.exec函数族
- l、v、p、e，**l与v二者选1，表示传入的是一个个参数还是一个参数集合**
- extern char **environ  
- `execl()`
- `execlp()`
- `execle()`
- `execv()`
- `execvp()`

## 5.用户权限及组权限
- u+s 当其他用户调用该可执行文件时，会切换成当前可执行文件的user的身份来执行
- g+s
- uid/gid
  - r real
  - e effective  
- 函数
  - `getuid` 返回 real  
  - `geteuid` 返回 effective
  - `getgid`
  - `getegid`
  - `setuid` 设置effective
  - `setgid` 设置effective
  - `setreuid` 交换 r e //是原子的交换
  - `setregid` 交换 r e
- **用户登录过程**
  - init->getty(fork+exec,000root用户权限)
  - getty->login(键入用户名后，exec)
  - login->shell(密码输入匹配成功，fork+exec，权限更改)
## 6.解释器文件
- 以“#!”跟一个执行文件开头，可以解析一堆命令
## 7.system()
- 理解：fork+exec+wait的封装
## 8.进程会计
- `acct()`查看进程执行的一些情况（时间，内存使用）
## 9.进程时间
- `times()`
## 10.守护进程  
- 会话session，标识sid
- 终端
- `setsid()`
- `getpgrp()`
- `getpgid()`
- `setpgid()`
- 单实例守护进程：锁文件 /var/run/"name".pid
- 启动脚本文件：/etc/rc*...

## 11.系统日志
- 守护进程没有终端，需要输出的时候可以输出到系统日志中
- syslogd服务
- `openlog()`
- `syslog()`
- `closelog()`
  