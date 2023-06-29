#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main()
{
    FILE* fp=NULL;
    fp=fopen("/tmp/out","r");
    if(fp==NULL)
    {
        perror("fopen");
        exit(1);
    }

    size_t linesize=0;
    char *line=NULL;
    while(1)
    {
        if(getline(&line,&linesize,fp)<0)
        {
            break;
        }
        printf("%ld\n",strlen(line));
        printf("%s",line);
        printf("%ld\n",linesize);
    }
    return 0;
}