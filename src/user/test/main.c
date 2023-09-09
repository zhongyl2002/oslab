// #include <stdio.h>
// #include <time.h>
// #include <fcntl.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/wait.h>
// #include <unistd.h>

// int main() {
//     long long int tt1 = syscall(228);
//     for(int i = 0; i < 100; i ++){
//         if(fork() == 0){
//             char* ch[] = {"echo", "abc 123", };
//             execv("echo", ch);
//             printf("execv failed\n");
//         }
//         wait(NULL);
//     }
//     long long int tt2 = syscall(228);
//     printf("花费时间：%ld\n", tt2 - tt1);
//     return 0;
// }


#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t child_pid = fork(); // 创建子进程

    if (child_pid == -1) {
        perror("无法创建子进程");
        return 1;
    }

    if (child_pid == 0) {
        // 这是子进程
        printf("这是子进程，PID：%d\n", getpid());
    } else {
        // 这是父进程
        printf("这是父进程，PID：%d，子进程的PID：%d\n", getpid(), child_pid);
    }

    return 0;
}
