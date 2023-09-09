#include<stdio.h>

void makedir(){
    for (int i = 0; i < 800; i++)
    {
        char str[20]; // 用于存储转换后的字符串
        sprintf(str, "%d", i);
        if(mkdir(str, 0) < 0){
            fprintf(stderr, "mkdir: %s failed to create\n", str);
            return -1;
        }
    }
}

int main(){
    int tt1 = syscall(228);

    makedir();

    int tt2 = syscall(228);
    // 154315 ms / 800个文件夹
    printf("花费时间为：%d ms / 800 个文件夹\n", tt2 - tt1);
    return 0;
}