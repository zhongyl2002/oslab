#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>

int main(int argc, char* argv[]){
    if(argc < 2){
        fprintf(stderr, "Usage: mkdir files...\n");
        exit(1);
    }

    int i = 1;
    for(; i < argc; i ++){
        if(mkdir(argv[i], 0) < 0){
            fprintf(stderr, "mkdir: %s failed to create\n", argv[i]);
            break;
        }
    }

    return 0;
}