#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    long long int tt1 = syscall(228);
    for(int i = 0; i < 100; i ++){
        if(fork() == 0){
            char* ch[] = {"echo", "abc 123", };
            execv("echo", ch);
            printf("execv failed\n");
        }
        wait(NULL);
    }
    long long int tt2 = syscall(228);
    printf("花费时间：%ld\n", tt2 - tt1);
    return 0;
}
