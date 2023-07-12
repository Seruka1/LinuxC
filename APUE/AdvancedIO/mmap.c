#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
//mmap实现读取一个文件的a的个数
int main(int argc,char** argv)
{
    int count = 0;
    char *str;
    struct stat statres;
    if (argc < 2)
    {
        fprintf(stderr,"Usage...\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if(fd<0)
    {
        perror("fopen()");
        exit(1);
    }

    if(fstat(fd, &statres)<0)
    {
        perror("fstat()");
        exit(1);
    }

    str=mmap(NULL, statres.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if(str==MAP_FAILED)
    {
        perror("mmap()");
        exit(1);
    }

    close(fd);
    for (int i = 0; i < statres.st_size;++i)
    {
        if(str[i]=='a')
        {
            ++count;
        }
    }
    printf("%d\n", count);
    munmap(str, statres.st_size);
    exit(0);
}