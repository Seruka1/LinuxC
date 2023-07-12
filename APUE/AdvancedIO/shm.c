#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<string.h>

#define MENSIZE 1024
int main()
{
    char* ptr;
    pid_t pid;
    ptr = mmap(NULL, MENSIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(ptr==MAP_FAILED)
    {
        perror("mmap()");
        exit(1);
    }
    pid = fork();
    if(pid<0)
    {
        perror("fork()");
        exit(1);
    }
    if(pid==0) //childwrite
    {
        strcpy(ptr, "Hello!");
        munmap(ptr, MENSIZE);
        exit(0);
    }
    wait(NULL);
    puts(ptr);
    munmap(ptr, MENSIZE);
    exit(0);
}