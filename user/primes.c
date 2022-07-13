#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 子进程主要是负责读的,因此要关闭写端
int subProcess(int *oldFd) {

    close(oldFd[1]);
    int fd[2];
    int primeNumber;
    int num;
    int pid;
    
    if (read(oldFd[0], &primeNumber, sizeof(int))) {
        //第一个数为质数
        printf("prime %d\n", primeNumber);
        
        //递归
        if (pipe(fd) == -1) {
            printf("pipe error.\n");
            exit(-1);
        }

        pid = fork();
        if (pid < 0) {
            printf("fork error.\n");
            exit(1);
        }
        else if (pid == 0) {
            subProcess(fd);
        } else {
            //主进程主要是负责写的，因此要关闭读端
            close(fd[0]);
            while (read(oldFd[0], &num, sizeof(int))) {
                if (num % primeNumber != 0) {
                    write(fd[1], &num, sizeof(int));
                }
            }
            close(oldFd[0]);
            close(fd[1]);
            wait(0);            
        } 
    } else {
        close(oldFd[0]);
    }
    exit(0);

}

int main(int argc, char *argv[])
{

    int pid;
    int fd[2];
    int i;
    if (pipe(fd) == -1) {
        printf("pipe error.\n");
        exit(-1);
    }

    pid = fork();
    if (pid < 0) {
        printf("fork error.\n");
        exit(1);
    }
    else if (pid == 0) {
        subProcess(fd);
    }
    else {
        //主进程主要是负责写的，因此要关闭读端
        close(fd[0]);
        for (i = 2; i <= 35; i++) {
            write(fd[1], &i, sizeof(int));
        }
        close(fd[1]);
        wait(0);
    }
    exit(0);

}
