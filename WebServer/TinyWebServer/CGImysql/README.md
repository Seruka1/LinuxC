## wsl下数据库的基本操作
#### Linux下命令
- `sudo service mysql start` 打开数据库
- `sudo service mysql stop`  关闭数据库
- `sudo service mysql restart` 重启数据库
- `sudo cat /etc/mysql/debian.cnf` 看系统给的账户和密码
- `systemctl status mysql.service`看mysql运行状态
- `sudo mysql -uroot -p` 连接mysql
- `sudo mysql_secure_installation`  设置密码安全
校验 & 数据库连接池
===============
数据库连接池
> * 单例模式，保证唯一
> * list实现连接池
> * 连接池为静态大小
> * 互斥锁实现线程安全

校验  
> * HTTP请求采用POST方式
> * 登录用户名和密码校验
> * 用户注册及多线程注册安全

