#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include<stdio.h>
#include<list>
#include<mysql/mysql.h>
#include<error.h>
#include<string>
#include<iostream>
#include<../lock/locker.h>
#include <../log/log.h>

using namespace std;

class connection_pool
{
public:
    //单例模式
    static connection_pool *GetInstance();

    void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log);

    MYSQL* GetConnection();  //获取一个数据库连接

    bool ReleaseConnection(MYSQL *conn);  //释放一个连接

    int GetFreeConn();           //获取空闲连接

    void DestroyPool();            //销毁所有连接

private:
    connection_pool();

    ~connection_pool();

    int m_MaxConn;   //最大连接数
    int m_CurConn;   //当前已经使用的连接数
    int m_FreeConn;   //获取空闲连接数
    locker lock;      //即使有信号量（还是会并行滴），修改连接池的时候仍然需要lock
    list<MYSQL *> connList;   //连接池
    sem reserve;     

public:
    string m_url;    //主机地址
    string m_Port;   //数据库端口号
    string m_User;   //登陆数据库用户名
    string m_PassWord;  //登陆数据库密码
    string m_DatabaseName;  //访问的数据库
    int m_close_log;         //日志开关
};

//封装连接的获取与释放，实现RAII(资源获取即初始化)，不用手动释放数据库连接
class connectionRAII
{
public:
    connectionRAII(MYSQL **con, connection_pool *connpool);

    ~connectionRAII();

private:
    MYSQL *conRAII;
    connection_pool *poolRAII;
};

#endif