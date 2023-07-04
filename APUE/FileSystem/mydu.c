//递归计算文件占用的磁盘空间
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glob.h>
#include <string.h>

#define PATHSIZE 1024

static int path_noloop(const char* path)
{
    char *pos;
    pos = strrchr(path, '/');
    if(pos==NULL)
    {
        exit(1);
    }
    else if(strcmp(pos+1,".")==0||strcmp(pos+1,"..")==0)
    {
        return 0;
    }
    return 1;
}

static int64_t mydu(const char* path)
{
    struct stat statres;
    char nextpath[PATHSIZE];
    glob_t globres;
    if(lstat(path,&statres)<0)
    {
        perror("lstat()");
        exit(1);
    }
    //非目录直接返回
    if(!S_ISDIR(statres.st_mode))
    {
        return statres.st_blocks;
    }
    //目录的话需要递归非隐藏文件与隐藏文件（除.与..）
    int64_t sum = statres.st_blocks;
    strncpy(nextpath, path, PATHSIZE);
    strncat(nextpath, "/*", PATHSIZE);
    if(glob(nextpath,0,NULL,&globres)<0)
    {
        perror("glob()");
        exit(1);
    }

    strncpy(nextpath, path, PATHSIZE);
    strncat(nextpath, "/.*", PATHSIZE);
    if (glob(nextpath, GLOB_APPEND, NULL, &globres) < 0)
    {
        perror("glob()");
        exit(1);
    }

    for (int i = 0; i < globres.gl_pathc;++i)
    {
        if(path_noloop(globres.gl_pathv[i]))
        {
            sum += mydu(globres.gl_pathv[i]);
        }
    }

    globfree(&globres);
    return sum;
}

int main(int argc,char **argv)
{
    if(argc<2)
    {
        fprintf(stderr, "Usage...\n");
        exit(1);
    }

    printf("%ld\t%s\n", mydu(argv[1]) / 2,argv[1]);

    exit(0);
}