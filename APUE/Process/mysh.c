#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <wait.h>
#include <glob.h>
#include <string.h>

#define DELIMS " \t\n"

//无法确定后续是否需要解析出别的数据，用结构体存数据
struct cmd_st
{
    glob_t globres;
};


static void prompt()
{
    printf("mysh-0.1$ ");
}

static void parse(char* line,struct cmd_st* res)
{
    char* tok;
    int flag=0;
    while(1)
    {
        tok=strsep(&line,DELIMS);
        if(tok==NULL)
        {
            break;
        }
        if(tok[0]=='\0')
        {
            continue;
        }
        glob(tok,GLOB_NOCHECK|GLOB_APPEND*flag,NULL,&res->globres);
        flag=1;
    }
}



int main()
{
    pid_t pid;
    struct cmd_st cmd;
    char* linebuf=NULL;
    size_t linebuf_size=0;
    while(1)
    {
        //打印提示符
        prompt();

        //从终端提取命令
        if(getline(&linebuf,&linebuf_size,stdin)<0)
        {
            break;
        }


        //解析命令行
        parse(linebuf,&cmd);


        if(0)   //内部命令暂不处理
        {
            //do sth
        }
        else
        {
            pid=fork();
            if(pid<0)
            {
                perror("fork()"); 
                exit(1);
            }
            if(pid==0)   //child
            {
                execvp(cmd.globres.gl_pathv[0],cmd.globres.gl_pathv);
                perror("execvp()");
                exit(1);
            }
            else
            {
                wait(NULL);
            }
        }
    }

    exit(0);
}