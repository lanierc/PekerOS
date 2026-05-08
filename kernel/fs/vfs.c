#include "vfs.h"
#include "kheap.h"

vfs_node_t *vfs_root = 0;

vfs_node_t *vfs_clone_node(vfs_node_t *node) {
    if (!node) return 0;
    vfs_node_t *res = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    memcpy(res, node, sizeof(vfs_node_t));
    return res;
}

unsigned int vfs_read(vfs_node_t *node, unsigned int offset, unsigned int size, unsigned char *buffer) {
    if (node->read != 0) {
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

unsigned int vfs_write(vfs_node_t *node, unsigned int offset, unsigned int size, unsigned char *buffer) {
    if (node->write != 0) {
        return node->write(node, offset, size, buffer);
    }
    return 0;
}

void vfs_open(vfs_node_t *node) {
    if (node->open != 0) {
        node->open(node);
    }
}

void vfs_close(vfs_node_t *node) {
    if (node->close != 0) {
        node->close(node);
    }
}

struct vfs_dirent *vfs_readdir(vfs_node_t *node, unsigned int index) {
    if ((node->flags & VFS_DIRECTORY) && (node->readdir != 0)) {
        return node->readdir(node, index);
    }
    return 0;
}

vfs_node_t *vfs_finddir(vfs_node_t *node, char *name) {
    if ((node->flags & VFS_DIRECTORY) && (node->finddir != 0)) {
        return node->finddir(node, name);
    }
    return 0;
}

vfs_node_t *vfs_get_node_by_path(vfs_node_t *base, const char *path) {
    if (!path) return 0;
    
    vfs_node_t *current;
    int i = 0;
    
    // Mutlak yol mu?
    if (path[0] == '/') {
        current = vfs_clone_node(vfs_root);
        i = 1; // '/' karakterini atla
    } else {
        if (!base) return 0;
        current = vfs_clone_node(base);
    }
    
    // Eğer yol sadece "/" ise veya boşsa (göreceli boşluk), mevcut düğümü döndür
    if (path[i] == '\0') return current;
    
    char name[128];
    while (path[i] != '\0') {
        int j = 0;
        while (path[i] != '/' && path[i] != '\0') {
            name[j++] = path[i++];
        }
        name[j] = '\0';
        
        if (path[i] == '/') i++; // '/' karakterini atla
        
        // "." ve boş isimleri atla (örn: //)
        if (j == 0 || (j == 1 && name[0] == '.')) continue;
        
        // ".." desteği (Şu anlık tam değil ama en azından bir üst dizin mantığı lazım)
        // Şimdilik sadece alt dizin araması yapıyoruz
        
        vfs_node_t *next = vfs_finddir(current, name);
        vfs_close(current);
        
        if (!next) return 0;
        current = next;
    }
    
    return current;
}
