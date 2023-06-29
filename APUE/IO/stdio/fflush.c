#include<stdio.h>
#include<stdlib.h>

/*
缓冲区的作用：大多数情况下是好事，合并系统调用
行缓冲：换行时候刷新，满了的时候刷新，强制刷新（终端设备是行缓冲）
全缓冲：满了的时候刷新，强制刷新（默认，只要不是终端设备）
无缓冲：如stderr，需要立即输出的内容

*/
int main()
{
    //注意 标准输出是行缓冲模式，如果没有\n 陷入死循环后不会把缓冲区的内容打印出来
    printf("Before Printf");

    fflush(stdout);
    while(1);

    printf("After Printf");


    exit(0);
}