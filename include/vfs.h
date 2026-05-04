#ifndef VFS_H
#define VFS_H

#include "common.h"

#define VFS_FILE      0x01
#define VFS_DIRECTORY 0x02
#define VFS_MOUNTPOINT 0x04

struct vfs_node;

typedef unsigned int (*read_type_t)(struct vfs_node*, unsigned int, unsigned int, unsigned char*);
typedef unsigned int (*write_type_t)(struct vfs_node*, unsigned int, unsigned int, unsigned char*);
typedef void (*open_type_t)(struct vfs_node*);
typedef void (*close_type_t)(struct vfs_node*);
typedef struct vfs_dirent* (*readdir_type_t)(struct vfs_node*, unsigned int);
typedef struct vfs_node* (*finddir_type_t)(struct vfs_node*, char *name);

typedef struct vfs_node {
    char name[128];
    unsigned int mask;      // İzinler (Şu anlık kullanılmıyor)
    unsigned int uid;
    unsigned int gid;
    unsigned int flags;     // Dosya tipi (VFS_FILE, VFS_DIRECTORY vb.)
    unsigned int inode;     // Dosya sistemine özel inode numarası
    unsigned int length;    // Byte cinsinden boyut
    unsigned int impl;      // Dosya sistemine özel ek bilgi
    
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    readdir_type_t readdir;
    finddir_type_t finddir;
    
    struct vfs_node *ptr;   // Mountpoint için root node
} vfs_node_t;

struct vfs_dirent {
    char name[128];
    unsigned int ino;
};

extern vfs_node_t *vfs_root;

// VFS ana fonksiyonları
unsigned int vfs_read(vfs_node_t *node, unsigned int offset, unsigned int size, unsigned char *buffer);
unsigned int vfs_write(vfs_node_t *node, unsigned int offset, unsigned int size, unsigned char *buffer);
void vfs_open(vfs_node_t *node);
void vfs_close(vfs_node_t *node);
struct vfs_dirent *vfs_readdir(vfs_node_t *node, unsigned int index);
vfs_node_t *vfs_finddir(vfs_node_t *node, char *name);
vfs_node_t *vfs_get_node_by_path(vfs_node_t *base, const char *path);
vfs_node_t *vfs_clone_node(vfs_node_t *node);

#endif
