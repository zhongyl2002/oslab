#pragma once

#include <common/defines.h>

/**
 * this file contains on-disk representations of primitives in our filesystem.
 */

#define BLOCK_SIZE 512

// maximum number of distinct block numbers can be recorded in the log header.
#define LOG_MAX_SIZE ((BLOCK_SIZE - sizeof(usize)) / sizeof(usize))

#define INODE_NUM_DIRECT   12
#define INODE_NUM_INDIRECT (BLOCK_SIZE / sizeof(u32))
#define INODE_PER_BLOCK    (BLOCK_SIZE / sizeof(InodeEntry))
#define INODE_MAX_BLOCKS   (INODE_NUM_DIRECT + INODE_NUM_INDIRECT)
#define INODE_MAX_BYTES    (INODE_MAX_BLOCKS * BLOCK_SIZE)

// the maximum length of file names, including trailing '\0'.
#define FILE_NAME_MAX_LENGTH 14

// inode types:
#define INODE_INVALID   0
#define INODE_DIRECTORY 1
#define INODE_REGULAR   2  // regular file
#define INODE_DEVICE    3

typedef u16 InodeType;

#define BIT_PER_BLOCK (BLOCK_SIZE * 8)


// 柱面组的组成
// [ SB | IB | BB | DB ]
// 柱面组结构体
#define logSize             63
#define cylinderGroupNum    4       // 柱面组数量
#define cylinderSize        936     // 一个柱面组的大小
#define cylinderInodeBase   1       // Inode开始
#define cylinderInodeSize   26      // Inode数量
#define cylinderBitmapBase  27      // Bitmap开始
#define cylinderBitmapSize  1       // Bitmap数量
#define cylinderDBBase      28      // data block开始
#define cylinderDBSize      908     // data block数量
#define recordBase          64
// mkfs only
// Size of file system in blocks
#define FSSIZE  1 + logSize + cylinderGroupNum * (1 + cylinderInodeSize + cylinderBitmapSize + cylinderDBSize)

typedef struct {
    u32 id;                 // 柱面组号
    u32 freeInodes;         // 空闲的Inode数量
} cylinderGroup;

static cylinderGroup cylinderGroups[cylinderGroupNum];

// disk layout:
// [ MBR block | super block | log blocks | inode blocks | bitmap blocks | data blocks ]
//
// `mkfs` generates the super block and builds an initial filesystem. The
// super block describes the disk layout.
typedef struct {
    u32 num_blocks;  // total number of blocks in filesystem.
    u32 num_data_blocks;
    u32 num_inodes;
    u32 num_log_blocks;  // number of blocks for logging, including log header.
    u32 log_start;       // the first block of logging area.
    u32 inode_start;     // the first block of inode area.
    u32 bitmap_start;    // the first block of bitmap area.
} SuperBlock;

// `type == INODE_INVALID` implies this inode is free.
typedef struct dinode {
    InodeType type;
    u16 major;                    // major device id, for INODE_DEVICE only.
    u16 minor;                    // minor device id, for INODE_DEVICE only.
    u16 num_links;                // number of hard links to this inode in the filesystem.
    u32 num_bytes;                // number of bytes in the file, i.e. the size of file.
    u32 addrs[INODE_NUM_DIRECT];  // direct addresses/block numbers.
    u32 indirect;                 // the indirect address block.
} InodeEntry;

// the block pointed by `InodeEntry.indirect`.
typedef struct {
    u32 addrs[INODE_NUM_INDIRECT];
} IndirectBlock;

// directory entry. `inode_no == 0` implies this entry is free.
typedef struct dirent {
    u16 inode_no;
    char name[FILE_NAME_MAX_LENGTH];
} DirEntry;

typedef struct {
    usize num_blocks;
    usize block_no[LOG_MAX_SIZE];
} LogHeader;

