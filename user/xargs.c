#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXLEN 32


int main(int argc, char *argv[])
{
    // for (int i = 0; i < argc; i++) {
    //     printf("%s\n", argv[i]);
    // }

    char buf[MAXARG*MAXLEN];
    char* params[MAXARG];
    int i;
    int len;

    if(argc < 1){
        printf("usage: xargs your_command\n");
        exit(1);
    }


    if (argc + 1 > MAXARG) {
        printf("too many args\n");
        exit(1);
    }
    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            params[i-1] = argv[i];
        }
    }
    else {
        params[0] = "echo";
    }

    params[argc] = 0;
    while (1) {

        i = 0;
        // read a line
        while (1) {

            len = read(0, &buf[i], 1); //读的是前面的参数
            if (len == 0 || buf[i] == '\n') break;
            i++;
        }
        if (i == 0) break;
        buf[i] = 0;

        //xargs后不接参数和接参数的情况略有不同, exec是可以处理带空格的情况的
        if (argc > 1) {
            params[argc-1] = buf;
        }
        else {
            params[argc] = buf;
        }

        if (fork() == 0) {
            exec(params[0],params);
            exit(0);
        } else {
            wait(0);
        }
    }
    exit(0);
}