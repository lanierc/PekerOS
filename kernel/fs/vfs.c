#include "vfs.h"
#include "kheap.h"

vfs_node_t *vfs_root = 0;

#define MAX_MOUNTS 16
static struct {
    char path[128];
    vfs_node_t *root;
} mount_table[MAX_MOUNTS];
static int mount_count = 0;

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
    if ((node->flags & VFS_MOUNTPOINT) && node->ptr) {
        return vfs_readdir(node->ptr, index);
    }

    if ((node->flags & VFS_DIRECTORY) && (node->readdir != 0)) {
        return node->readdir(node, index);
    }
    return 0;
}

vfs_node_t *vfs_finddir(vfs_node_t *node, char *name) {
    if ((node->flags & VFS_MOUNTPOINT) && node->ptr) {
        return vfs_finddir(node->ptr, name);
    }
    if ((node->flags & VFS_DIRECTORY) && (node->finddir != 0)) {
        vfs_node_t *res = node->finddir(node, name);
        if (res) {
            // Mount tablosunda bu yolu ara
            // Basitlik icin simdilik sadece '/mnt' kontrolu
            if (strcmp(name, "mnt") == 0) {
                for (int i = 0; i < mount_count; i++) {
                    if (strcmp(mount_table[i].path, "/mnt") == 0) {
                        res->flags |= VFS_MOUNTPOINT;
                        res->ptr = mount_table[i].root;
                        break;
                    }
                }
            }
        }
        return res;
    }
    return 0;
}

void vfs_mount(char *path, vfs_node_t *root) {
    if (mount_count >= MAX_MOUNTS) return;
    
    strcpy(mount_table[mount_count].path, path);
    mount_table[mount_count].root = root;
    mount_count++;
    
    put_str("[VFS] Basariyla mount edildi: ");
    put_str(path);
    put_str("\n");
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

        // Sembolik bag cozunurlemesi (Follow Symlink)
        int symlink_count = 0;
        while ((next->flags & VFS_SYMLINK) && symlink_count < 8) {
            char link[128];
            memset(link, 0, 128);
            vfs_read(next, 0, 128, (unsigned char*)link);
            
            vfs_node_t *target;
            if (link[0] == '/') {
                target = vfs_get_node_by_path(vfs_root, link);
            } else {
                // Goreceli baglar icin 'current' (parent) lazim ama 'current' yukarida close edildi.
                // Basitlik icin su anlik sadece mutlak veya ayni dizindeki baglari destekliyoruz.
                // Gercek VFS'de parent node saklanmalidir.
                target = vfs_get_node_by_path(0, link); // 0 ise vfs_root'tan baslar
            }
            
            vfs_close(next);
            if (!target) return 0;
            next = target;
            symlink_count++;
        }
        
        current = next;
    }
    
    return current;
}
