#include"log.h"

char *EM_logLevelGet(const int level) // 得到当前输入等级level的字符串
{
    if(level==LOGLEVEL_DEBUG)
    {
        return (char *)"DEBUG";
    }
    else if(level==LOGLEVEL_INFO)
    {
        return (char *)"INFO";
    }
    else if(level==LOGLEVEL_WARN)
    {
        return (char *)"WARN";
    }
    else if(level==LOGLEVEL_ERROR)
    {
        return (char *)"ERROR";
    }
    else 
    {
        return (char *)"UNKNOWN";
    }
 }

 void EM_log(const int level,const char* fun,const int line,const char* fmt,...)//输出日志文件
 {
    #ifdef OPEN_LOG   //判断开关
    va_list arg;
    va_start(arg, fmt);
    char buf[1024];                        // 创建缓存字符数组
    vsnprintf(buf, sizeof(buf), fmt, arg); // 赋值 ftm 格式的 arg 到 buf
    // vsnprintf的第一个参数是一个传出参数，我们把格式化字符串 fmt 和可变参数 ... 格式化成一个字符数组 buf
    va_end(arg);
    if (level >= LOG_LEVEL) // 判断当前日志等级，与程序日志等级状态对比
    {
        printf("[%s]\t[%s %d]: %s \n", EM_logLevelGet(level), fun, line, buf);
    }
#endif
 }