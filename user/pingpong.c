#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int pid;
    char buf[1];
    int fd[2];
    char* pSend = "p"; //parent send
    char* cSend = "c"; //child send
    //1. 创建管道, 创建成功后fd[0]是读端, fd[1]是写端
    if (pipe(fd) == -1) {
        printf("pipe error.\n");
        exit(-1);
    }

    // 新建子进程
    pid = fork();
    if (pid < 0) {
        printf("fork error.\n");
        exit(1);
    }
    else if (pid == 0) { //为子进程
        read(fd[0], buf, sizeof(buf));
        printf("%d: received ping\n", getpid());
        write(fd[1], cSend, 1);

    }
    else { //为父进程, 给子进程发送消息
        write(fd[1], pSend, 1);
        read(fd[0], buf, sizeof(buf));
        printf("%d: received pong\n", getpid());
    }
    exit(0);

}
