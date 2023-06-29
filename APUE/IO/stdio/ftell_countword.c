#include<stdio.h>
#include<stdlib.h>

//通过文件偏移量获取文件数据大小
int main(int argc,char** argv)
{
    if(argc<2)
    {
        fprintf(stderr,"Usage...\n");
        exit(1);
    }

    FILE* fp;
    fp=fopen(argv[1],"r");
    if(fp==NULL)
    {
        perror("fopen()");
        exit(1);
    }

    fseek(fp,0,SEEK_END);
    printf("%ld\n",ftell(fp));

    exit(0);

}