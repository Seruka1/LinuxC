#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
/*
-y:year
-m:month
-d:day
-H:hour
-M:minute
-S:second
*/
// 这里只处理了文件，没有递归去写入文件夹内的文件  可借助glob来实现
#define TIMESTRSIZE 1024
#define FMTSTRSIZE 1024
int main(int argc, char **argv)
{
    time_t stamp;
    stamp = time(NULL);
    int c;
    struct tm *struct_tm;
    struct_tm = localtime(&stamp);
    char timestr[TIMESTRSIZE];
    char fmtstr[FMTSTRSIZE];
    fmtstr[0] = '\0';
    FILE *fp = stdout;

    while (1)
    {
        c = getopt(argc, argv, "-H:Sy:Mmd");
        if (c < 0)
        {
            break;
        }
        switch (c)
        {
        case 1:
        {
            fp = fopen(argv[optind - 1], "w");
            if (fp == NULL)
            {
                perror("fopen()");
                fp = stdout;
            }
            break;
        }
        case ('H'):
        {
            if (strcmp(optarg, "12") == 0)
            {
                strncat(fmtstr, "%I(%P) ", FMTSTRSIZE);
            }
            else if (strcmp(optarg, "24") == 0)
            {
                strncat(fmtstr, "%H ", FMTSTRSIZE);
            }
            else
            {
                fprintf(stderr, "Invalid argument of -H\n");
            }
            break;
        }
        case ('M'):
        {
            strncat(fmtstr, "%M ", FMTSTRSIZE);
            break;
        }
        case ('S'):
        {
            strncat(fmtstr, "%S ", FMTSTRSIZE);
            break;
        }
        case ('y'):
        {
            if (strcmp(optarg, "2") == 0)
            {
                strncat(fmtstr, "%y ", FMTSTRSIZE);
            }
            else if (strcmp(optarg, "4") == 0)
            {
                strncat(fmtstr, "%Y ", FMTSTRSIZE);
            }
            else
            {
                fprintf(stderr, "Invalid argument of -y\n");
            }
            break;
        }
        case ('m'):
        {
            strncat(fmtstr, "%m ", FMTSTRSIZE);
            break;
        }
        case ('d'):
        {
            strncat(fmtstr, "%d ", FMTSTRSIZE);
            break;
        }
        default:
        {
            break;
        }
        }
    }

    strncat(fmtstr, "\n", FMTSTRSIZE);
    strftime(timestr, TIMESTRSIZE, fmtstr, struct_tm);
    fputs(timestr, fp);
    if (fp != stdout)
    {
        fclose(fp);
    }
    exit(0);
}