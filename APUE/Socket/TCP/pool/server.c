#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <errno.h>

#include "proto.h"




int main()
{
    socket();

    setsockopt();  //复用socket地址

    bind();

    listen();

    while(1)
    {
        suspend();
        scan_pool();

        add_1_server();

        del_1_server();


        //printf pool
    }
    close();
    exit(0);

}