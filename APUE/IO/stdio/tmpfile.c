#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main()
{
    FILE* tmpfp;
    tmpfp=tmpfile();

    const char* oribuf="2023/06/29";
    int SIZE=0;
    while(*(oribuf+SIZE)!=0)
    {
        ++SIZE;
    }
    printf("%d\n",SIZE);

    fwrite(oribuf,1,SIZE,tmpfp);
    fseek(tmpfp,0,SEEK_SET);

    FILE* fp;
    fp=fopen("/tmp/out","w");
    char desBuf[SIZE];
    int n=fread(desBuf,1,SIZE,tmpfp);
    fwrite(desBuf,1,SIZE,fp);

    fclose(tmpfp);
    fclose(fp);

    exit(0);

}