# 文件系统

## 目标

实例：类ls的实现,如myls -l -a -i -n

## 一、目录和文件

### 1. 获取文件属性

- stat 通过文件路径获取属性
- fstat 通过文件描述符获取属性
- lstat 面对符号链接文件时，获取的是符号🔗文件l的属性,而stat获取的是链接对象的属性
- 注意：在unix中文件大小 `size`只是一个属性，不一定代表文件真正的大小(与文件系统相关)

### 2. 文件访问权限

st_mode（rwxr--r--）总共16位，用于表示文件类型、文件访问权限、特殊权限位（粘住位、组用户设置位）

- 文件类型 dcb-lsp（7种文件类型，用3位表示）
  - 目录文件
  - 字符设备文件
  - 块设备文件
  - 常规文件
  - 符号链接文件
  - 套接字文件
  - 命名管道文件

### 3. umask

  作用：防止产生权限过松的文件

### 4. 文件权限的更改/管理

  在一个文件中想要运行一个脚本，那么可能需要改变文件的权限

- chmod

  - chmod a+x ??x ??x ??x
  - chmod u+x ??x ??? ???
  - chmod g+x ??? ??x ???
  - chmod o+x ??? ??? ??x
- fchmod

### 5. 粘住位

  t位：目前一般用于目录的设置，**只有对该目录具有写权限的用户并且满足下列条件之一**，才可以删除或者重命名该目录下的文件：

- 拥有此文件
- 拥有此目录
- 是超级用户

### 6. 文件系统：FAT,UFS

  文件或数据的存储和管理

### 7. 硬链接，符号链接

- link (命令) 创建硬链接其实就是在目录项中添加一条映射，存在限制：不能给分区建立，不能给目录建立。符号链接优点：可跨分区，可以给目录建立。
- ln() => ln
- unlink() 删除一个文件的硬连接 但并不是删除文件 只有当一个文件的硬链接数为0 且没有进程占用该文件时一个文件才有被删除的可能（数据可被随意改写）

### 8. utime

  可以更改文件最后读的时间和改的时间。

### 9. 目录的创建与销毁

- mkdir
- rmdir

### 10. 更改当前工作目录

- chdir
- fchdir

### 11. 分析目录/读取目录内容

**单独调用**

- glob解析模式/通配符（*）

**组合调用**

- opendir()
- closedir()
- readdir()
- seekdir()
- telldir()

## 二、系统数据文件和信息

1. /etc/passwd

   - getpwuid();
   - getpwnam();
2. /etc/group

   - getgrgid();
   - getgrgrnam();
3. /etc/shadow

   - getspnam();
   - crypt();
   - getpass();
   - 密码加盐
4. 时间戳
   **time_t=>struct_tm=>char***

- time() 从kernel中取出时间戳(以秒为单位)，存储从1970年到现在经过了多少秒
- gntime() 将时间戳转换为struct_tm 格林威治时间
- localtime() 将时间戳转换为struct_tm 本地时间
- mktime() jaing struct_tm结构体转换为时间戳，还可以检查是否溢出
- strftime(); 格式化时间字符串
  ```
  time_t stamp;
  time(&stamp);
  stamp = time(NULL);
  tm = localtime(&stamp);
  strftime(buf,BUFSIZE,"%Y-%m-%d",tm);
  puts(buf);
  ```
- tail -f 循环读取文件

## 三、进程环境

1. main函数

- `int main(int argc,char** argv)`

2. 进程的终止：

- 正常终止：
  - 从main函数返回
  - 调用 `exit`
  - 调用 `_exit`或者 `_Exit`，**调用_exit/_Exit直接返回内核  不执行钩子函数以及清理缓冲区**
  - 最后一个线程从其启动例程返回
  - 最后一个线程调用 `pthread_exit`
- 异常终止：
  - 调用abort
  - 接到一个信号并终止
  - 最后一个线程对其取消请求作出响应
- 钩子函数 `atexit()`
  当执行exit时，逆序调用注册的钩子函数。

3. 命令行参数的分析

- `getopt()`
- `getopt_long()`

4. 环境变量**KEY==VALUE**
   - Linux export命令查看环境变量
   - `getenv()`
   - `setenv()`
   - `putenv()`
5. C程序的存储空间布局`pmap(1)`
6. 库

- 动态库
- 静态库
- 手工装载库
  - `dlopen`
  - `dlclose`
  - `dlerror`
  - `dlsym`

7. 函数跳转

```
适用场景： 在树结构中查找元素，找到后直接回到第一次调用处(跨函数),不用一层一层返回
```

- `setjmp`
- `longjmp`

8. 资源的获取与控制

- getrlimit
- setrlimit
- ulimit
