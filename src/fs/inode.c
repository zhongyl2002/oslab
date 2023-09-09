#include <common/string.h>
#include <core/arena.h>
#include <core/console.h>
#include <core/physical_memory.h>
#include <core/sched.h>
#include <fs/inode.h>

// this lock mainly prevents concurrent access to inode list `head`, reference
// count increment and decrement.
static SpinLock lock;
static ListNode head;

static const SuperBlock *sblock;
static const BlockCache *cache;
static Arena arena;

// return which block `inode_no` lives on.
static INLINE usize to_block_no(usize inode_no) {
    int gid = inode_no / inodePerCylinder;
    return recordBase + gid * cylinderSize + sblock->inode_start + ((inode_no % inodePerCylinder) / (INODE_PER_BLOCK));
}

// return the pointer to on-disk inode.
static INLINE InodeEntry *get_entry(Block *block, usize inode_no) {
    return ((InodeEntry *)block->data) + (inode_no % INODE_PER_BLOCK);
}

// return address array in indirect block.
static INLINE u32 *get_addrs(Block *block) {
    return ((IndirectBlock *)block->data)->addrs;
}

// used by `inode_map`.
static INLINE void set_flag(bool *flag) {
    if (flag != NULL)
        *flag = true;
}

// initialize inode tree.
void init_inodes(const SuperBlock *_sblock, const BlockCache *_cache) {
    ArenaPageAllocator allocator = {.allocate = kalloc, .free = kfree};

    init_spinlock(&lock, "inode tree");
    init_list_node(&head);
    sblock = _sblock;
    cache = _cache;
    init_arena(&arena, sizeof(Inode), allocator);

    if (ROOT_INODE_NO < sblock->num_inodes)
        inodes.root = inodes.get(ROOT_INODE_NO);
    else
        printf("(warn) init_inodes: no root inode.\n");
}

// initialize in-memory inode.
static void init_inode(Inode *inode) {
    init_spinlock(&inode->lock, "inode");
    init_rc(&inode->rc);
    init_list_node(&inode->node);
    inode->inode_no = 0;
    inode->valid = false;
}

// see `inode.h`.
static usize inode_alloc(OpContext *ctx, tp tap) {
    assert(tap.type != INODE_INVALID);

    // 为目录分配inode节点
    if(tap.type == INODE_DIRECTORY){
        // printf("In dir branch\n");
        // inode空闲数最多的组
        int mmax = getFreeinode(0), bestG = 0;
        printf("group 1 free num : %d\n", mmax);
        for (int i = 1; i < cylinderGroupNum; i++)
        {
            int tmp = getFreeinode(i);
            printf("group %d free num : %d\n", i, tmp);
            if(mmax < tmp){
                mmax = tmp;
                bestG = i;
            }
        }
        printf("bestG = %d, mmin = %d\n", bestG, mmax);
        for (usize ino = inodePerCylinder * bestG; ino < inodePerCylinder * (bestG + 1); ino++) {
            // printf("inode = %d\n", ino);
            usize block_no = to_block_no(ino);
            // printf("block_no = %d\n", block_no);
            Block *block = cache->acquire(block_no);
            InodeEntry *inode = get_entry(block, ino);

            if (inode->type == INODE_INVALID) {
                // printf("find proper in iter %d\n", ino);
                memset(inode, 0, sizeof(InodeEntry));
                inode->type = tap.type;
                inode->num_links = 1;
                cache->sync(NULL, block);
                cache->release(block);
                // catInode(recordBase + cylinderSize + cylinderInodeBase);
                // catInode(block_no);
                return ino;
            }

            cache->release(block);
        }

    }
    // 为文件分配节点，tp.pid != 0
    else if(tap.type == INODE_REGULAR){
        int gid = tap.pid / inodePerCylinder;
        for (usize ino = inodePerCylinder * gid; ino < inodePerCylinder * (gid + 1); ino++) {
            usize block_no = to_block_no(ino);
            Block *block = cache->acquire(block_no);
            InodeEntry *inode = get_entry(block, ino);

            if (inode->type == INODE_INVALID) {
                memset(inode, 0, sizeof(InodeEntry));
                inode->type = tap.type;
                cache->sync(ctx, block);
                cache->release(block);
                printf("new file(inode = %d) will be placed in group %d\n", ino, gid);
                return ino;
            }

            cache->release(block);
        }
    }else{
        for (usize ino = 1; ino < inodePerCylinder; ino++) {
            usize block_no = to_block_no(ino);
            Block *block = cache->acquire(block_no);
            InodeEntry *inode = get_entry(block, ino);

            if (inode->type == INODE_INVALID) {
                memset(inode, 0, sizeof(InodeEntry));
                inode->type = tap.type;
                cache->sync(ctx, block);
                cache->release(block);
                return ino;
            }

            cache->release(block);
        }
    }

    PANIC("failed to allocate inode on disk");
}

// see `inode.h`.
static void inode_sync(OpContext *ctx, Inode *inode, bool do_write) {
    usize block_no = to_block_no(inode->inode_no);
    Block *block = cache->acquire(block_no);
    InodeEntry *entry = get_entry(block, inode->inode_no);

    if (inode->valid && do_write) {
        memcpy(entry, &inode->entry, sizeof(InodeEntry));
        cache->sync(ctx, block);
    } else if (!inode->valid) {
        memcpy(&inode->entry, entry, sizeof(InodeEntry));
        inode->valid = true;
    }

    cache->release(block);
}

// see `inode.h`.
static void inode_lock(Inode *inode) {
    assert(inode->rc.count > 0);
    acquire_spinlock(&inode->lock);

    if (!inode->valid)
        inode_sync(NULL, inode, false);
    assert(inode->entry.type != INODE_INVALID);
}

// see `inode.h`.
static void inode_unlock(Inode *inode) {
    assert(holding_spinlock(&inode->lock));
    assert(inode->rc.count > 0);
    release_spinlock(&inode->lock);
}

// see `inode.h`.
static Inode *inode_get(usize inode_no) {
    assert(inode_no > 0);
    assert(inode_no < sblock->num_inodes);
    acquire_spinlock(&lock);

    Inode *inode = NULL;
    for (ListNode *cur = head.next; cur != &head; cur = cur->next) {
        Inode *inst = container_of(cur, Inode, node);
        if (inst->rc.count > 0 && inst->inode_no == inode_no) {
            increment_rc(&inst->rc);
            inode = inst;
            break;
        }
    }

    if (inode == NULL) {
        inode = alloc_object(&arena);
        assert(inode != NULL);
        init_inode(inode);
        inode->inode_no = inode_no;
        increment_rc(&inode->rc);
        merge_list(&head, &inode->node);
    }

    release_spinlock(&lock);

    return inode;
}

// see `inode.h`.
static void inode_clear(OpContext *ctx, Inode *inode) {
    InodeEntry *entry = &inode->entry;

    for (usize i = 0; i < INODE_NUM_DIRECT; i++) {
        usize addr = entry->addrs[i];
        if (addr != 0)
            cache->free(ctx, addr);
    }
    memset(entry->addrs, 0, sizeof(entry->addrs));

    usize iaddr = entry->indirect;
    if (iaddr != 0) {
        Block *block = cache->acquire(iaddr);
        u32 *addrs = get_addrs(block);
        for (usize i = 0; i < INODE_NUM_INDIRECT; i++) {
            if (addrs[i] != 0)
                cache->free(ctx, addrs[i]);
        }

        cache->release(block);
        cache->free(ctx, iaddr);
        entry->indirect = 0;
    }

    entry->num_bytes = 0;
    inode_sync(ctx, inode, true);
}

// see `inode.h`.
static Inode *inode_share(Inode *inode) {
    acquire_spinlock(&lock);
    increment_rc(&inode->rc);
    release_spinlock(&lock);
    return inode;
}

// see `inode.h`.
static void inode_put(OpContext *ctx, Inode *inode) {
    acquire_spinlock(&lock);
    bool is_last = inode->rc.count <= 1 && inode->entry.num_links == 0;

    if (is_last) {
        inode_lock(inode);
        release_spinlock(&lock);

        inode_clear(ctx, inode);
        inode->entry.type = INODE_INVALID;
        inode_sync(ctx, inode, true);

        inode_unlock(inode);
        acquire_spinlock(&lock);
    }

    if (decrement_rc(&inode->rc)) {
        detach_from_list(&inode->node);
        free_object(inode);
    }
    release_spinlock(&lock);
}

// this function is private to inode layer, because it can allocate block
// at arbitrary offset, which breaks the usual file abstraction.
//
// retrieve the block in `inode` where offset lives. If the block is not
// allocated, `inode_map` will allocate a new block and update `inode`, at
// which time, `*modified` will be set to true.
// the block number is returned.
//
// NOTE: caller must hold the lock of `inode`.
static usize inode_map(OpContext *ctx, Inode *inode, usize offset, bool *modified) {
    InodeEntry *entry = &inode->entry;
    usize index = offset / BLOCK_SIZE;

    if (index < INODE_NUM_DIRECT) {
        if (entry->addrs[index] == 0) {
            entry->addrs[index] = (u32)cache->alloc(ctx, (inode->inode_no / inodePerCylinder));
            printf("分配第%d个直接块 %d, 到组%d\n", index, entry->addrs[index], (entry->addrs[index] - recordBase) / cylinderSize);
            set_flag(modified);
        }
        return entry->addrs[index];
    }

    index -= INODE_NUM_DIRECT;
    assert(index < INODE_NUM_INDIRECT);

    if (entry->indirect == 0) {
        entry->indirect = (u32)cache->alloc(ctx, (inode->inode_no / inodePerCylinder));
        printf("分配indirect块，%d, 到组%d\n", entry->indirect, (entry->indirect - recordBase) / cylinderSize);
        set_flag(modified);
    }

    Block *block = cache->acquire(entry->indirect);
    u32 *addrs = get_addrs(block);

    if (addrs[index] == 0) {
        addrs[index] = (u32)cache->alloc(ctx, ((inode->inode_no / inodePerCylinder) + 1 + index / otherGroupMax) % cylinderGroupNum);
        printf("分配间接块%d, 块%d, 到组%d\n", index, addrs[index], (addrs[index] - recordBase) / cylinderSize);
        cache->sync(ctx, block);
        set_flag(modified);
    }

    usize addr = addrs[index];
    cache->release(block);
    return addr;
}

// see `inode.h`.
static usize inode_read(Inode *inode, u8 *dest, usize offset, usize count) {
    InodeEntry *entry = &inode->entry;

    if (inode->entry.type == INODE_DEVICE) {
        assert(inode->entry.major == 1);
        return (usize)console_read(inode, (char *)dest, (isize)count);
    }
    if (count + offset > entry->num_bytes)
        count = entry->num_bytes - offset;
    usize end = offset + count;
    assert(offset <= entry->num_bytes);

    assert(end <= entry->num_bytes);
    assert(offset <= end);

    usize step = 0;
    for (usize begin = offset; begin < end; begin += step, dest += step) {
        bool modified = false;
        usize block_no = inode_map(NULL, inode, begin, &modified);
        assert(!modified);

        Block *block = cache->acquire(block_no);
        usize index = begin % BLOCK_SIZE;
        step = MIN(end - begin, BLOCK_SIZE - index);
        memmove(dest, block->data + index, step);
        cache->release(block);
    }
    return count;
}

// see `inode.h`.
static usize inode_write(OpContext *ctx, Inode *inode, u8 *src, usize offset, usize count) {
    InodeEntry *entry = &inode->entry;
    usize end = offset + count;
    if (inode->entry.type == INODE_DEVICE) {
        assert(inode->entry.major == 1);
        return (usize)console_write(inode, (char *)src, (isize)count);
    }
    // printf("offset = %u, num_byte = %u\n", offset, entry->num_bytes);
    // if(offset > entry->num_bytes){
    //     printf("offset = %u, num_byte = %u\n", offset, entry->num_bytes);
    //     assert(offset <= entry->num_bytes);
    // }
    // printf("end = %d\n", end);
    assert(offset <= entry->num_bytes);
    // if(end > INODE_MAX_BYTES){
    //     printf("end = %d\n", end);
    //     printf("INODE_NUM_INDIRECT = %d, INODE_MAX_BLOCKS = %d, INODE_MAX_BYTES = %d\n", 
    //             INODE_NUM_INDIRECT, INODE_MAX_BLOCKS, INODE_MAX_BYTES);
    //     assert(end <= INODE_MAX_BYTES);
    // }
    assert(end <= INODE_MAX_BYTES);
    assert(offset <= end);

    usize step = 0;
    bool modified = false;
    for (usize begin = offset; begin < end; begin += step, src += step) {
        usize block_no = inode_map(ctx, inode, begin, &modified);
        // if(begin == offset){
        //     printf("file(inode = %d) data block will be placed in block %d(group %d)\n", inode->inode_no, block_no, (block_no - recordBase) / cylinderSize);
        // }
        Block *block = cache->acquire(block_no);
        usize index = begin % BLOCK_SIZE;
        step = MIN(end - begin, BLOCK_SIZE - index);
        memmove(block->data + index, src, step);
        cache->sync(ctx, block);
        cache->release(block);
    }

    if (end > entry->num_bytes) {
        entry->num_bytes = (u32)end;
        modified = true;
    }
    if (modified)
        inode_sync(ctx, inode, true);
    return count;
}

// see `inode.h`.
static usize inode_lookup(Inode *inode, const char *name, usize *index) {
    InodeEntry *entry = &inode->entry;
    assert(entry->type == INODE_DIRECTORY);

    DirEntry dentry;
    for (usize offset = 0; offset < entry->num_bytes; offset += sizeof(dentry)) {
        inode_read(inode, (u8 *)&dentry, offset, sizeof(dentry));
        if (dentry.inode_no != 0 && strncmp(name, dentry.name, FILE_NAME_MAX_LENGTH) == 0) {
            if (index != NULL)
                *index = offset / sizeof(dentry);
            return dentry.inode_no;
        }
    }

    return 0;
}

// see `inode.h`.
static usize inode_insert(OpContext *ctx, Inode *inode, const char *name, usize inode_no) {
    InodeEntry *entry = &inode->entry;
    assert(entry->type == INODE_DIRECTORY);

    DirEntry dentry;
    usize offset = 0;
    for (; offset < entry->num_bytes; offset += sizeof(dentry)) {
        inode_read(inode, (u8 *)&dentry, offset, sizeof(dentry));
        if (dentry.inode_no == 0)
            break;
    }

    dentry.inode_no = (u16)inode_no;
    strncpy(dentry.name, name, FILE_NAME_MAX_LENGTH);
    inode_write(ctx, inode, (u8 *)&dentry, offset, sizeof(dentry));
    return offset / sizeof(dentry);
}

// see `inode.h`.
static void inode_remove(OpContext *ctx, Inode *inode, usize index) {
    InodeEntry *entry = &inode->entry;
    assert(entry->type == INODE_DIRECTORY);

    DirEntry dentry;
    usize offset = index * sizeof(dentry);
    if (offset >= entry->num_bytes)
        return;

    memset(&dentry, 0, sizeof(dentry));
    inode_write(ctx, inode, (u8 *)&dentry, offset, sizeof(dentry));
}

/* Paths. */

/* Copy the next path element from path into name.
 *
 * Return a pointer to the element following the copied one.
 * The returned path has no leading slashes,
 * so the caller can check *path=='\0' to see if the name is the last one.
 * If no name to remove, return 0.
 *
 * Examples:
 *   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
 *   skipelem("///a//bb", name) = "bb", setting name = "a"
 *   skipelem("a", name) = "", setting name = "a"
 *   skipelem("", name) = skipelem("////", name) = 0
 */
static const char *skipelem(const char *path, char *name) {
    const char *s;
    int len;

    while (*path == '/')
        path++;
    if (*path == 0)
        return 0;
    s = path;
    while (*path != '/' && *path != 0)
        path++;
    len = (int)(path - s);
    if (len >= FILE_NAME_MAX_LENGTH)
        memmove(name, s, FILE_NAME_MAX_LENGTH);
    else {
        memmove(name, s, (usize)len);
        name[len] = 0;
    }
    while (*path == '/')
        path++;
    return path;
}

/* Look up and return the inode for a path name.
 *
 * If parent != 0, return the inode for the parent and copy the final
 * path element into name, which must have room for DIRSIZ bytes.
 * Must be called inside a transaction since it calls iput().
 */
static Inode *namex(const char *path, int nameiparent, char *name, OpContext *ctx) {
    Inode *ip, *next;

    if (*path == '/')
        ip = inodes.get(1);
    else
        ip = inodes.share(thiscpu()->proc->cwd);

    while ((path = skipelem(path, name)) != 0) {
        inodes.lock(ip);
        if (ip->entry.type != INODE_DIRECTORY) {
            inodes.unlock(ip);
            bcache.begin_op(ctx);
            inodes.put(ctx, ip);
            bcache.end_op(ctx);
            return 0;
        }
        if (nameiparent && *path == '\0') {
            // Stop one level early.
            inodes.unlock(ip);
            return ip;
        }
        if (*path == '.' && *(path + 1) == '\0') {
            inodes.unlock(ip);
            return ip;
        }
        if (inodes.lookup(ip, name, 0) == 0) {
            inodes.unlock(ip);
            inodes.put(ctx, ip);
            return 0;
        }
        next = inodes.get(inodes.lookup(ip, name, 0));
        inodes.unlock(ip);
        inodes.put(ctx, ip);
        ip = next;
    }
    if (nameiparent) {
        bcache.begin_op(ctx);
        inodes.put(ctx, ip);
        bcache.end_op(ctx);
        return 0;
    }
    return ip;
}

Inode *namei(const char *path, OpContext *ctx) {
    char name[FILE_NAME_MAX_LENGTH];
    return namex(path, 0, name, ctx);
}

Inode *nameiparent(const char *path, char *name, OpContext *ctx) {
    return namex(path, 1, name, ctx);
}

/*
 * Copy stat information from inode.
 * Caller must hold ip->lock.
 */
void stati(Inode *ip, struct stat *st) {
    // FIXME: support other field in stat
    st->st_dev = 1;
    st->st_ino = ip->inode_no;
    st->st_nlink = ip->entry.num_links;
    st->st_size = ip->entry.num_bytes;

    switch (ip->entry.type) {
        case INODE_REGULAR: st->st_mode = S_IFREG; break;
        case INODE_DIRECTORY: st->st_mode = S_IFDIR; break;
        case INODE_DEVICE: st->st_mode = 0; break;
        default: PANIC("unexpected stat type %d. ", ip->entry.type);
    }
}
InodeTree inodes = {
    .alloc = inode_alloc,
    .lock = inode_lock,
    .unlock = inode_unlock,
    .sync = inode_sync,
    .get = inode_get,
    .clear = inode_clear,
    .share = inode_share,
    .put = inode_put,
    .read = inode_read,
    .write = inode_write,
    .lookup = inode_lookup,
    .insert = inode_insert,
    .remove = inode_remove,
};

/*
    SUMMARY:
        主要实现了inode的结构和功能，实现inode层、directory层、pathname层
*/