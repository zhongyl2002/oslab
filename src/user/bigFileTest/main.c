#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<string.h>

void help(){
    printf("this file is used to test big file layout~\n");
}

int main(int argc, char* argv[]){
    int fd;

    if(argc == 2 && !strcmp(argv[1], "-h")){
        help();
        return 0;
    }

    // ms信息
    int tt1 = syscall(228);

    if((fd = open("bigFileTest_f", O_WRONLY | O_CREAT | O_APPEND, 0666)) < 0){
        fprintf(stderr, "cat: cannot open bigFileTest\n");
        exit(1);
    }

    const char *data = "12345678";
    // char data[] = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
    size_t data_len = sizeof(data);
    
    for (size_t i = 0; i < (70 * 1024) / data_len; i++) {
        ssize_t bytes_written = write(fd, data, data_len);
        if (bytes_written == -1) {
            perror("写入文件时出错");
            close(fd);
            return 1;
        }
    }

    close(fd);

    int tt2 = syscall(228);
    // 62711 ms / 124块
    printf("大致花费时间(每次8字节有输出)为：%d ms / %d块\nNOTE:测试可能与每次写入的大小和输出有关\n\t无输出每次8字节写入时为 55817 ms / 140块\n\t无输出每次8字节写入时为 2428ms / 140块\n", tt2 - tt1, 140);

    return 0;
}
