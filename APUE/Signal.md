# 并发
**同步与异步**  
异步事件的处理：查询法，通知法 

## 信号
### 1. 信号的概念
  信号是软件中断。  
  信号的响应依赖于中断。
### 2. signal()
  `void (*signal(int signum,void(*func)(int)))(int);`  
  信号会打断阻塞的系统调用
### 3. 信号的不可靠
### 4. 可重入函数
### 5. 信号的响应过程
### 6. 常用函数
- `kill()`
- `raise()`
- `alarm()`
- `pause()`
- `abort()`
- `system()`
- `sleep()`
### 7. 信号集
### 8. 信号屏蔽字/pengding集的处理
### 9. 扩展
- `sigsuspend()`
- `sigaction()`
- `setitimer()`
### 10. 实时信号

## 线程