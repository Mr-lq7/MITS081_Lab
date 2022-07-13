#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int i;
    if(argc < 2 || argc > 3){
        fprintf(2, "Usage: sleep number ticks...\n");
        exit(1);
    }

    i = 1;
    int ticks = atoi(argv[i]);
    if (ticks < 0) {
        fprintf(2, "number ticks are invalid. \n");
        exit(1);
    }
    sleep(ticks);
    exit(0);
}
