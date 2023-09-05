#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>

char buf[512];

void cat(int fd){
    int n;

    while((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(1, buf, n) != n) {
            fprintf(stderr, "cat: write error\n");
            exit(1);
        }
    }
    if(n < 0){
        fprintf(stderr, "cat: read error\n");
        exit(1);
    }
}

int main(int argc, char* argv[]){
    if(argc <= 1){
        cat(0);
        exit(0);
    }

    int fd, i;

    for(i = 1; i < argc; i ++){
        if((fd = open(argv[i], O_RDONLY)) < 0){
            fprintf(stderr, "cat: cannot open %s\n", argv[i]);
            exit(1);
        }
        cat(fd);
        close(fd);
    }

    return 0;
}