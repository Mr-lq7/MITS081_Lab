#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


//切除路径的最后一级，例如 /xx/xx, 就会被xx截出来例如xx.py就直接把xx.py截出来
char* fmtname(char *path)
{
    // static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    return p;

    // Return blank-padded name.
    // if(strlen(p) >= DIRSIZ)
    //     return p;
    // memmove(buf, p, strlen(p));
    // memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
    // return buf;
}


void find(char *path, char *targetFileName) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){

        //是文件
        case T_FILE:

            //输出文件名
            if (strcmp(fmtname(path), targetFileName) == 0) {
                printf("%s\n", path);
            }
            break;

        case T_DIR: 
            
            // printf("%s\n", path);

            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("find: path too long\n");
                break;
            }

            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/'; //buf的前半部分存路径,以/隔开

            // 循环读取文件夹下的文件,文件名为de,name
            while(read(fd, &de, sizeof(de)) == sizeof(de)){

                // printf("%s\n", de.name);
                if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;

                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;

                if(stat(buf, &st) < 0){
                    printf("ls: cannot stat %s\n", buf);
                    continue;
                }

                // 递归方式1
                int before = strlen(path);
                char *tmp = path + before;
                *tmp++ = '/';
                strcpy(tmp, fmtname(path));
                int after = strlen(path);
                find(buf, targetFileName);
                //回溯
                memset(path+before, 0, after-before);

                //递归方式2
                // find(buf, targetFileName);            
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{


    if (argc != 3) {
        fprintf(2, "args error. \n");
        exit(1);
    }

    find(argv[1], argv[2]);

    exit(0);
}
