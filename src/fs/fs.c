#include <fs/block_device.h>
#include <fs/cache.h>
#include <fs/defines.h>
#include <fs/fs.h>
#include <fs/inode.h>

u32 xint(u32 x) {
    u32 y;
    unsigned char *a = (unsigned char *)&y;
    a[0] = x;
    a[1] = x >> 8;
    a[2] = x >> 16;
    a[3] = x >> 24;
    return y;
}

void superBlockCheck(SuperBlock* sb){
    printf("sb.num_blocks = %d (%d)\n", xint(sb->num_blocks), sb->num_blocks);
    printf("sb->num_data_blocks = %d\n", sb->num_data_blocks);
    printf("sb->num_inodes = %d\n", sb->num_inodes);
    printf("sb->num_log_blocks = %d\n", sb->num_log_blocks);
    printf("sb->log_start = %d\n", sb->log_start);
    printf("sb->inode_start = %d\n", sb->inode_start);
    printf("sb->bitmap_start = %d\n", sb->bitmap_start);
}

void initCylinderGroups(){
    for (int i = 0; i < cylinderGroupNum; i++)
    {
        cylinderGroups[i].id = i;
        cylinderGroups[i].freeInodes = getFreeinode(i);
        printf("第%d柱面组空闲inode为：%d\n", cylinderGroups[i].freeInodes);
    }
}


void init_filesystem() {
    init_block_device();
    const SuperBlock *sblock = get_super_block();
    initCylinderGroups();
    init_bcache(sblock, &block_device);
    init_inodes(sblock, &bcache);
}
