#include <stdio.h>
#include <stdlib.h>

#define SIZE 5

int main(int argc, char **argv)
{

    FILE *fps, *fpd;
    char buf[SIZE];
    int n;

    if (argc < 3)
    {
        fprintf(stderr, "Usage:...\n");
        exit(1);
    }

    fps = fopen(argv[1], "r");
    if (fps == NULL)
    {
        perror("fopen()");
        exit(1);
    }

    fpd = fopen(argv[2], "w");
    if (fpd == NULL)
    {
        perror("fopen()");
        exit(1);
    }

    while (1)
    {
        if ((n = fread(buf, 1, SIZE, fps)) <= 0)
        {
            break;
        }
        if (fwrite(buf, 1, n, fpd) == EOF)
        {
            perror("fwrite()");
            exit(1);
        }
    }

    fclose(fpd);
    fclose(fps);

    exit(0);
}