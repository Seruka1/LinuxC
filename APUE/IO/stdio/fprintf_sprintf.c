#include <stdio.h>
#include <stdlib.h>

/*****************
 *sprintf atoi fprintf
 *sprintf 可以看作是 atoi的反向操作
 * **************/
int main()
{
    char buf[1024];
    int y = 2023;
    int m = 6;
    int d = 28;
    sprintf(buf, "%d-%d-%d", y, m, d);
    printf("%s\n", buf);

    exit(0);
}