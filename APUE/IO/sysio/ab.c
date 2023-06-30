#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    //系统调用IO直接切换到内核输出，标准IO需要刷新缓冲，因此下面这个程序会先输出1024个b然后1024个a然后一个b一个a
    for(int i=0;i<1025;++i)
    {
        putchar('a');
        write(1,"b",1);
    }

    return 0;
}