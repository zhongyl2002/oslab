#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include "../../fs/defines.h"

#define BSIZE         BLOCK_SIZE

int fsfd;

void rsect(int sec, void *buf) {
    if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
        perror("lseek");
        exit(1);
    }
    if (read(fsfd, buf, BSIZE) != BSIZE) {
        perror("read");
        exit(1);
    }
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void help(){
    printf("命令功能为格式化输出disk指定块的内容\n");
    printf("输入参数为:\n\t1.块的地址,即第x块\n\t2.块的类型(\n\t\t1.super block\n\t\t2.inode block\n\t\t3.bitmap block\n\t\t4.data block)\n");
}

void catBitmap(int blockNo){
    uint8_t testbuf[512];
    rsect(blockNo, testbuf);
    for(int i = 0; i < 512; i ++)
    {
        for (int j = 0; j < 8; j++)
        {
            if(testbuf[i] & (1 << (7 - j))){
                printf("第%d(i = %d, j = %d)块被使用\n", i * 8 + j, i, j);
            }
        }
    }
}

void catInode(int blockNo){
    char buf[512];
    int ret = 0;
    rsect(blockNo, buf);
    for(int j = 0; j < INODE_PER_BLOCK; j ++){
        InodeEntry* ie = ((InodeEntry*)buf) + j;
        printf("第%dinode节点:", j);
        switch (ie->type)
        {
        case INODE_INVALID:
            printf("type = INODE_INVALID, ");
            break;
        case INODE_DIRECTORY:
            printf("type = INODE_DIRECTORY, ");
            break;
        case INODE_REGULAR:
            printf("type = INODE_REGULAR, ");
            break;
        default:
            printf("type = INODE_DEVICE, ");
        }
        printf("num_links = %d, num_bytes = %d\n\t", ie->num_links, ie->num_bytes);
        for (int i = 0; i < INODE_NUM_DIRECT; i++)
        {
            printf("direct[%d] = %d ", ie->addrs[i]);
        }
        printf("\n\t");
        printf("indirect = %d\n", ie->indirect);
    }
}

int main(int argc, char* argv[]){
    fsfd = open("fs.img", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fsfd < 0) {
        perror("fs.img");
        exit(1);
    }

    if(argc < 2) {
        fprintf(stderr, "too few parameters\n");
        return -1;
    }


    if(!strcmp(argv[1], "-h")){
        help();
        return 0;
    }
    if(!strcmp(argv[2], "2")){
        catInode(atoi(argv[1]));
        return 0;
    } 
    if(!strcmp(argv[2], "3")) {
        catBitmap(atoi(argv[1]));
        return 0;
    }
    return 0;
}