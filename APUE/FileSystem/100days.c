#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#define TIMESTRSIZE 1024
int main()
{
    time_t stamp;
    stamp = time(NULL);

    struct tm *struct_tm;
    struct_tm = localtime(&stamp);
    char timestr[TIMESTRSIZE];
    strftime(timestr, TIMESTRSIZE, "NOW:%Y-%m-%d", struct_tm);
    puts(timestr);

    struct_tm->tm_mday += 100;
    mktime(struct_tm);

    strftime(timestr, TIMESTRSIZE, "100 days later:%Y-%m-%d", struct_tm);
    puts(timestr);
}