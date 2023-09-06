#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

void help(){
    printf("命令功能为格式化输出disk指定块的内容\n");
    printf("输入参数为:\n\t1.块的地址\n\t2.块的类型\n");
}

int main(int argc, char* argv[]){
    if(argc < 2) {
        fprintf(stderr, "too few parameters\n");
    }
    if(!strcmp(argv[1], "-h")){
        help();
    }
    return 0;
}