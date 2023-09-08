#include <driver/sd.h>
#include <fs/block_device.h>

// TODO: we should read this value from MBR block.
#define BLOCKNO_OFFSET 0x20800

static void sd_read(usize block_no, u8 *buffer) {
    struct buf b;
    b.blockno = (u32)block_no + BLOCKNO_OFFSET;
    b.flags = 0;
    sdrw(&b);
    memcpy(buffer, b.data, BLOCK_SIZE);
}

static void sd_write(usize block_no, u8 *buffer) {
    struct buf b;
    b.blockno = (u32)block_no + BLOCKNO_OFFSET;
    b.flags = B_DIRTY | B_VALID;
    memcpy(b.data, buffer, BLOCK_SIZE);
    sdrw(&b);
}

static u8 sblock_data[BLOCK_SIZE];
BlockDevice block_device;

void init_block_device() {
    sd_init();
    sd_read(recordBase, sblock_data);

    block_device.read = sd_read;
    block_device.write = sd_write;
}

const SuperBlock *get_super_block() {
    return (const SuperBlock *)sblock_data;
}

int getFreeinode(int id){
    u8 buf[BSIZE];
    int ret = 0;
    for (int i = 0; i < cylinderInodeSize; i++)
    {
        sd_read(recordBase + id * cylinderSize + cylinderInodeBase + i, buf);
        // printf("read %d block\n", recordBase + id * cylinderSize + cylinderInodeBase + i);
        for(int j = 0; j < INODE_PER_BLOCK; j ++){
            InodeEntry* ie = ((InodeEntry*)buf) + j;
            if(ie->num_links == 0){
                ret ++;
            }
        }
    }
    return ret;
}

void catInode(int blockNo){
    printf("\n\nCatInode start block %d\n", blockNo);
    char buf[512];
    sd_read(blockNo, buf);
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
    printf("CatInode end\n");
}


/*
总结：block_device.h、block_device.c文件
    使用block_device结构体，初始化该结构体的读写方法（.read和.write属性），
    使得其他文件可以使用这两个属性读写磁盘
*/
