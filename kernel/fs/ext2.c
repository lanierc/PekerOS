#include "ext2.h"
#include "kheap.h"
#include "vfs.h"

static uint32_t ext2_read_block(ext2_fs_t *fs, uint32_t block, uint8_t *buf) {
    return vfs_read(fs->dev, block * fs->block_size, fs->block_size, buf);
}

static void ext2_read_inode(ext2_fs_t *fs, uint32_t ino, struct ext2_inode *inode) {
    uint32_t group = (ino - 1) / fs->sb.s_inodes_per_group;
    uint32_t index = (ino - 1) % fs->sb.s_inodes_per_group;
    uint32_t block = fs->bgdt[group].bg_inode_table + (index * fs->sb.s_inode_size) / fs->block_size;
    uint32_t offset = (index * fs->sb.s_inode_size) % fs->block_size;
    
    uint8_t *buf = kmalloc(fs->block_size);
    ext2_read_block(fs, block, buf);
    memcpy(inode, buf + offset, sizeof(struct ext2_inode));
    kfree(buf);
}

// Forward declarations for VFS functions
static uint32_t ext2_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
static struct vfs_dirent *ext2_readdir(vfs_node_t *node, uint32_t index);
static vfs_node_t *ext2_finddir(vfs_node_t *node, char *name);

static vfs_node_t *ext2_inode_to_vfs(ext2_fs_t *fs, uint32_t ino, char *name) {
    struct ext2_inode einode;
    ext2_read_inode(fs, ino, &einode);

    vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));
    strcpy(node->name, name);
    node->inode = ino;
    node->length = einode.i_size;
    node->impl = (uint32_t)fs;
    
    // Dosya tipi tespiti
    if ((einode.i_mode & 0xF000) == 0x4000) node->flags = VFS_DIRECTORY;
    else if ((einode.i_mode & 0xF000) == 0xA000) node->flags = VFS_SYMLINK;
    else node->flags = VFS_FILE;
    
    node->read = ext2_read;
    node->readdir = ext2_readdir;
    node->finddir = ext2_finddir;
    
    return node;
}

static uint32_t ext2_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    ext2_fs_t *fs = (ext2_fs_t *)node->impl;
    struct ext2_inode einode;
    ext2_read_inode(fs, node->inode, &einode);
    
    if (offset >= einode.i_size) return 0;
    if (offset + size > einode.i_size) size = einode.i_size - offset;
    
    // Sembolik bag mi?
    if (node->flags & VFS_SYMLINK) {
        // Hizli symlink mi? (Boyut < 60 ise i_block icindedir)
        if (einode.i_size < 60) {
            uint32_t to_copy = (size < einode.i_size - offset) ? size : (einode.i_size - offset);
            memcpy(buffer, (uint8_t*)einode.i_block + offset, to_copy);
            return to_copy;
        }
    }

    uint32_t read_bytes = 0;
    uint8_t *block_buf = kmalloc(fs->block_size);
    
    while (read_bytes < size) {
        uint32_t block_idx = (offset + read_bytes) / fs->block_size;
        uint32_t block_off = (offset + read_bytes) % fs->block_size;
        uint32_t to_read = fs->block_size - block_off;
        if (to_read > (size - read_bytes)) to_read = size - read_bytes;
        
        // Simdilik sadece direct block'lari destekliyoruz (i_block[0-11])
        if (block_idx < 12) {
            ext2_read_block(fs, einode.i_block[block_idx], block_buf);
            memcpy(buffer + read_bytes, block_buf + block_off, to_read);
        }
        
        read_bytes += to_read;
    }
    
    kfree(block_buf);
    return read_bytes;
}

static vfs_node_t *ext2_finddir(vfs_node_t *node, char *name) {
    ext2_fs_t *fs = (ext2_fs_t *)node->impl;
    struct ext2_inode einode;
    ext2_read_inode(fs, node->inode, &einode);
    
    uint8_t *buf = kmalloc(fs->block_size);
    for (int i = 0; i < 12 && einode.i_block[i]; i++) {
        ext2_read_block(fs, einode.i_block[i], buf);
        struct ext2_dir_entry *entry = (struct ext2_dir_entry *)buf;
        uint32_t offset = 0;
        
        while (offset < fs->block_size) {
            char entry_name[256];
            memcpy(entry_name, entry->name, entry->name_len);
            entry_name[entry->name_len] = '\0';
            
            if (strcmp(entry_name, name) == 0) {
                vfs_node_t *res = ext2_inode_to_vfs(fs, entry->inode, entry_name);
                kfree(buf);
                return res;
            }
            
            offset += entry->rec_len;
            entry = (struct ext2_dir_entry *)(buf + offset);
        }
    }
    
    kfree(buf);
    return 0;
}

static struct vfs_dirent *ext2_readdir(vfs_node_t *node, uint32_t index) {
    ext2_fs_t *fs = (ext2_fs_t *)node->impl;
    struct ext2_inode einode;
    ext2_read_inode(fs, node->inode, &einode);
    
    static struct vfs_dirent dirent;
    uint8_t *buf = kmalloc(fs->block_size);
    uint32_t current_index = 0;

    for (int i = 0; i < 12 && einode.i_block[i]; i++) {
        ext2_read_block(fs, einode.i_block[i], buf);
        struct ext2_dir_entry *entry = (struct ext2_dir_entry *)buf;
        uint32_t offset = 0;
        
        while (offset < fs->block_size) {
            if (entry->inode != 0) {
                if (current_index == index) {
                    memcpy(dirent.name, entry->name, entry->name_len);
                    dirent.name[entry->name_len] = '\0';
                    dirent.ino = entry->inode;
                    kfree(buf);
                    return &dirent;
                }
                current_index++;
            }
            
            offset += entry->rec_len;
            if (entry->rec_len == 0) break;
            entry = (struct ext2_dir_entry *)(buf + offset);
        }
    }
    
    kfree(buf);
    return 0; 
}

vfs_node_t *ext2_init(vfs_node_t *dev) {
    put_str("[EXT2] Inisiyalizasyon basliyor...\n");
    ext2_fs_t *fs = kmalloc(sizeof(ext2_fs_t));
    fs->dev = dev;
    
    // Superblock is 1024 bytes, starting at 1024
    uint8_t *sb_buf = kmalloc(1024);
    vfs_read(dev, 1024, 1024, sb_buf);
    
    memcpy(&fs->sb, sb_buf, sizeof(struct ext2_superblock));
    kfree(sb_buf);
    
    if (fs->sb.s_magic != EXT2_SUPER_MAGIC) {
        put_str("[EXT2] Hata: Gecersiz Magic Number: 0x");
        put_hex(fs->sb.s_magic);
        put_str("\n");
        kfree(fs);
        return 0;
    }
    
    fs->block_size = 1024 << fs->sb.s_log_block_size;
    fs->groups_count = (fs->sb.s_blocks_count + fs->sb.s_blocks_per_group - 1) / fs->sb.s_blocks_per_group;
    
    // Read BGDT
    fs->bgdt = kmalloc(fs->groups_count * sizeof(struct ext2_group_desc));
    uint32_t bgdt_offset = (fs->block_size == 1024 ? 2048 : fs->block_size);
    vfs_read(dev, bgdt_offset, fs->groups_count * sizeof(struct ext2_group_desc), (uint8_t *)fs->bgdt);
             
    put_str("[EXT2] Dosya sistemi mount edildi. Blok boyutu: ");
    put_int(fs->block_size);
    put_str("\n");
    
    return ext2_inode_to_vfs(fs, 2, "/"); // Root inode is always 2
}
