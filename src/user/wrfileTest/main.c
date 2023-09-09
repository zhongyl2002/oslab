#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>

char buf[512];

void help(){
    printf("this file is used to test writing & reading small file\n");
}

void readFile(char* str){
    int n, fd;
    
    if((fd = open(str, O_RDONLY)) < 0){
        fprintf(stderr, "cat: cannot open %s\n", str);
        exit(1);
    }

    while((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(1, buf, n) != n) {
            fprintf(stderr, "readFile:write error\n");
            close(fd);
            exit(1);
        }
    }
    if(n < 0){
        fprintf(stderr, "readFile:read error\n");
        close(fd);
        exit(1);
    }

    close(fd);
}


void writeFile(char* str){
    int fd;
    
    if((fd = open(str, O_WRONLY | O_CREAT | O_APPEND)) < 0){
        fprintf(stderr, "cat: cannot open %s\n", str);
        exit(1);
    }

    char data[] = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
    size_t data_len = sizeof(data);
    
    // 写4KB内容
    for (size_t i = 0; i < (4 * 1024) / data_len; i++) {
        ssize_t bytes_written = write(fd, data, data_len);
        if (bytes_written == -1) {
            perror("writeFile:write error\n");
            close(fd);
            return 1;
        }
    }
    printf("\n");

    close(fd);
}

int main(int argc, char* argv[]){
    if(argc == 2 && !strcmp("-h", argv[1])){
        help();
        return 0;
    }

    if(mkdir("1", 0) < 0){
        fprintf(stderr, "mkdir: %s failed to create\n", "1");
        return -1;
    }

    if (chdir("1/") < 0){
        fprintf(stderr, "cannot cd %s\n", buf + 3);
        return -1;
    }

    int tt1 = syscall(228);
    char str[10];
    for (int i = 0; i < 100; i++)
    {
        sprintf(str, "%d", i);
        writeFile(str);
    }
    int tt2 = syscall(228);
    // 17952 ms/ 100 个小文件
    printf("写小文件花费时间为:%d ms/ 100 个小文件\n", tt2 - tt1);
    
    int tt3 = syscall(228);
    for (int i = 0; i < 100; i++)
    {
        sprintf(str, "%d", i);
        readFile(str);
    }
    int tt4 = syscall(228);
    // 19020 ms/ 100 个小文件
    printf("读小文件花费时间为:%d ms/ 100 个小文件\n", tt4 - tt3);

    return 0;
}