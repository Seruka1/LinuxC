#include<stdio.h>
#include<stdlib.h>

#define SIZE 5


int main(int argc,char** argv)
{

    FILE *fps, *fpd;
    char buf[SIZE];

    if(argc<3)
    {
        fprintf(stderr, "Usage:...\n");
        exit(1);
    }

    fps = fopen(argv[1], "r");
    if(fps==NULL)
    {
        perror("fopen()");
        exit(1);
    }

    fpd = fopen(argv[2], "w");
    if(fpd==NULL)
    {
        perror("fopen()");
        exit(1);
    }

    while(1)
    {
        if(fgets(buf,SIZE,fps)==NULL)
        {
            break;
        }
        if(fputs(buf,fpd)==EOF)
        {
            perror("fputc");
            exit(1);
        }
    }

    fclose(fpd);
    fclose(fps);

    exit(0);
}