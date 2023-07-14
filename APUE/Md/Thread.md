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
### 互斥量
**锁住的是一段代码而不是一个变量**
- 动态初始化：`pthread_mutex_init()`  
  静态初始化：`pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER`
- `pthread_mutex_destroy()`
- `pthread_mutex_lock()`,阻塞
- `pthread_mutex_trylock()`，非阻塞
- `pthread_mutex_unlock()`
- `pthread_once()`，**动态模块的单词初始化函数**
- 临界区内跳转到临界区外一定要注意解锁
### 条件变量
**条件变量是配合锁来使用的，锁上发现不满足条件那么就wait，等待状态变化。条件变量可以解决互斥量进行盲等的问题，即实现了通知法，通知互斥量什么时候上锁**
- `pthread_cond_t`
- `pthread_cond_init()`
- `pthread_cond_destroy()`
- `pthread_cond_broadcast()`，广播所有线程
- `pthread_cond_signal()`，通知任一线程
- `pthread_cond_wait()`，**等通知+抢锁**
- `pthread_cond_timewait()`
### 信号量
**通过互斥量与条件变量的配合我们可以实现信号量。信号量像是一个激活函数，当这个变量超过阈值时，将会触发条件变量给互斥量上锁**
- **互斥量可以看作二进制信号量** 
- 哲学家问题 
### 读写锁
- 读锁->共享锁，写锁->互斥锁
- 读者写者问题
## 4. 线程属性
- pthread_attr_init()
- pthread_attr_destory()
- pthread_attr_setstacksize() 其他请见 man pthread_attr_init 的 see also
### 线程同步的属性
#### 互斥量属性
- pthread_mutexattr_init()
- pthread_mutexattr_destory()
- clone 进程 线程 不分家，**有些问题用子进程解决不太好，用线程解决也不太好**
  - 跨进程设置锁
  - pthread_mutexattr_getshared()
  - pthread_mutexattr_setshared()
- pthread_mutexattr_gettype()，互斥量有四种类型
- pthread_mutexattr_settype()
#### 条件变量的属性
- pthread_condattr_init()
- pthread_condattr_destory()
#### 读写锁的属性
## 5. 重入（reentry）
**多线程IO**都支持线程安全（给缓冲区上锁），如果你就是要调用单进程单线程，那么可以用unlock版本
- getchar_unlocked
## 6. 线程与信号
**多线程每个线程都有一个mask与pending，而进程只有一个pending**
- pthread_sigmask()
- sigwait()
- pthread_kill()
- 线程与fork

## openmp（另一套线程标准） -->www.OpenMp.org

