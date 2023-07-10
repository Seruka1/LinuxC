# Thread
## 1. 线程的概念
会话是用来承载进程组的，里面可以有一个或多个进程，一个线程中可以有一个或多个线程 **线程的本质就是一个正在运行的函数 ，线程没有主次之分(main函数 也只是一个main线程)，多个线程之间共享内存，**

**posix线程是一套标准，而不是实现，我们主要讨论这套标准**
- 线程标识`pthread_t`类型不确定
- `pthread_equal()`
- `pthread_self()`
- ps xam可以看进程中的线程
## 2. 线程的一生
### 线程创建
  - `pthread_create()`，**线程的调度取决于调度器的策略**
### 线程终止
1. 线程从启动例程返回，返回值就是线程的退出码
2. 线程可以被同一进程的其他线程取消
3. 线程调用`pthread_exit()`函数
- `pthread_join()`相当于进程中的`wait()`.
### 栈的清理
**类似钩子函数，程序只要正常终止，钩子函数就会被逆序调用，push 与 pop 可以指定操作**
- `pthread_cleanup_push()`
- `pthread_cleanup_pop()`  
### 线程的取消选项
- 多线程任务 有时需要取消部分任务(线程)
- `pthread_cancel()`
- 取消有2种状态
  - 不允许
  - 允许
    - 异步cancel
    - **推迟cancel(默认) 推迟到cancel点再响应**  
      cancel点 ： POSIX定义的cancel点,**都是可能引发阻塞的系统调用**
- `pthread_setcancelstate()`:设置是否允许取消
- `pthread_setcanceltype()`:设置cancel类型
- `pthread_testcancel()`:本函数什么都不做，就是一个取消点
### 线程分离
- `pthread_detach()`，create后不想管线程的状态就可以detach  
**detach后再join会报错**
 
## 3. 线程同步
## 4. 线程属性
- 线程同步的属性
## 5. 重入
## 6. 线程与信号
- 线程与fork