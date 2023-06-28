#include <stdio.h>
#include <stdlib.h>
// fgetc fputc版本的mycp
int main(int argc, char **argv)
{
    // 命令行传参要判断参数个数合不合理
    if (argc < 3)
    {
        fprintf(stderr, "Usage:%s <src_file> <dest_file>\n", argv[0]);
        exit(1);
    }
    FILE *fps, *fpd;
    int ch;
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
        ch = fgetc(fps);
        if (ch == EOF)
        {
            break;
        }
        if (fputc(ch, fpd) == EOF)
        {
            perror("fputc()");
            exit(1);
        }
    }

    // 先关闭被依赖的对象
    fclose(fpd);
    fclose(fps);

    exit(0);
}