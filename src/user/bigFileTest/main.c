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

    if((fd = open("bigFileTest_f", O_WRONLY | O_CREAT | O_APPEND, 0666)) < 0){
        fprintf(stderr, "cat: cannot open bigFileTest\n");
        exit(1);
    }

    const char *data = "12345678";
    size_t data_len = sizeof(data);
    
    for (size_t i = 0; i < 8000; i++) {
        ssize_t bytes_written = write(fd, data, data_len);
        if (bytes_written == -1) {
            perror("写入文件时出错");
            close(fd);
            return 1;
        }
    }

    close(fd);

    return 0;
}
