#ifndef FAFS_H
#define FAFS_H

#include "common.h"

// FAFS (PekerOS File System) Ayarları
#define FAFS_MAGIC       0xFAFC
#define FAFS_BLOCK_SIZE  512
#define FAFS_MAX_INODES  128
#define FAFS_TOTAL_BLOCKS 4096 // 2 MB Sanal Disk

// Inode Tipleri
#define FAFS_TYPE_FREE 0
#define FAFS_TYPE_FILE 1
#define FAFS_TYPE_DIR  2

// Dosya Sistemi Üstverisi (Superblock)
struct fafs_superblock {
    unsigned int magic;
    unsigned int total_blocks;
    unsigned int total_inodes;
    unsigned int free_blocks;
    unsigned int free_inodes;
    unsigned int root_inode;
};

// Inode (Dosya/Klasör Meta Verisi)
struct fafs_inode {
    unsigned int type;           // 1: Dosya, 2: Klasör
    unsigned int size;           // Dosya boyutu
    unsigned int blocks[12];     // Veri blok numaraları (Maks 12 * 512 = 6KB dosya boyutu, basitlik için)
};

// Klasör Girdisi (Directory Entry)
struct fafs_dir_entry {
    unsigned int inode;          // Hangi inode'a ait?
    char name[28];               // Dosya/Klasör adı
};

// FAFS Fonksiyonları
void fafs_init(void);
void fafs_format(void);
int fafs_create(const char *name, int is_dir);
int fafs_write(const char *name, const char *data, int len);
int fafs_read(const char *name, char *buffer, int max_len);
void fafs_list_dir(void);

#endif
