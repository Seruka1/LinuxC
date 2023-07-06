#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#define FNAME "/tmp/out"

static int deamonize()
{
    int fd;
    pid_t pid;
    pid=fork();
    if(pid<0)
    {
        return -1;
    }
    if(pid>0)
    {
        exit(0);
    }

    fd=open("/dev/null",O_RDWR);
    if(fd<0)
    {
        return -1;
    }
    dup2(fd,0);
    dup2(fd,1);
    dup2(fd,2);
    if (fd > 2){
        close(fd);
    }
    setsid();//脱离终端
    //umask();
    chdir("/");
    return 0;
}

//创建一个守护进程一直往FNAME中输出整数
int main()
{
    FILE* fp;

    //开启日志服务来输出提示信息
    openlog("print i",LOG_PID,LOG_DAEMON);

    if(deamonize())
    {
        syslog(LOG_ERR,"init failed!");
    }
    else 
    {
        syslog(LOG_INFO,"succeeded");
    }

    fp=fopen(FNAME,"w+");
    if(fp==NULL)
    {
        syslog(LOG_ERR,"write file failed!");
        exit(1);
    }

    syslog(LOG_INFO,"%s opened",FNAME);

    for(int i = 0; ;i++){
        fprintf(fp,"%d\n",i);
        fflush(NULL);
        syslog(LOG_DEBUG,"%d 写入",i);
        sleep(1);
    }

    closelog();
    fclose(fp);
    exit(0);

}