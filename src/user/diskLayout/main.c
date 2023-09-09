#include "../../fs/defines.h"
#include"stdio.h"
#include"string.h"

void help(){
    printf("this shell command is for printing disk layout~\n");
}

void printHead(){
    printf("\thead:\n");
    printf("\t\t| MBR(1 0-0) | log(%d %d-%d) |\n", logSize, 1, logSize);
}

void printCylinder(int gid, int start){
    printf("\tcylinder %d:\n", gid);
    printf("\t\t| superBlock(%d %d-%d) | inodeBlock(%d %d-%d) | bitmapBlock(%d %d-%d) | dataBlock(%d %d-%d) |\n",
            1, start, start,
            cylinderInodeSize, start + cylinderInodeBase, start + cylinderInodeBase + cylinderInodeSize - 1,
            1, start + cylinderBitmapBase, start + cylinderBitmapBase + cylinderBitmapSize - 1,
            cylinderDBSize, start + cylinderDBBase, start + cylinderDBBase + cylinderDBSize - 1);
}

int main(int argc, char* argv[]){
    if(argc == 2 && !strcmp(argv[1], "-h")){
        help();
        return 0;
    }
    printf("disk layout is:\n");
    printHead();
    for (int i = 0; i < cylinderGroupNum; i++)
    {
        printCylinder(i, recordBase + i * cylinderSize);
    }
    return 0;
}