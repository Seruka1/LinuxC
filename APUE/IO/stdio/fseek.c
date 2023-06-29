#include<stdio.h>
#include<stdlib.h>

#define SIZE 1024

int main()
{
    FILE* fp=NULL;
    char buf[SIZE];
    buf[0]='A';
    buf[1]='A';
    buf[2]='A';
    buf[3]='A';

    fp=fopen("/tmp/out","w+");
    if(fp==NULL)
    {
        perror("fopen()");
        exit(1);
    }

    int i=0;
    while(i<10)
    {
        unsigned long n=fwrite(buf,1,4,fp);
        fseek(fp,-n,SEEK_CUR);
        unsigned long len=fread(buf,1,n,fp);
        printf("%lu\n",len);
        fseek(fp,0,SEEK_END);
        i++;
    }

    fseek(fp,1024,SEEK_CUR);

    exit(0);
}