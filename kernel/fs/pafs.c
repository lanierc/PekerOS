#include "pafs.h"
#include "kheap.h"
#include "ata.h"
#include "vfs.h"
#include "spinlock.h"

// Dosyanın üst kısmına ekleyin:
static void pafs_save_superblock(void);
static void pafs_save_bitmap(void);
static void pafs_save_inode(int ino);

static spinlock_t pafs_lock = SPINLOCK_INIT;

// Bellek içi hızlı erişim işaretçileri
static struct pafs_superblock *sb;
static struct pafs_inode *inodes;
static unsigned char *block_bitmap; // 0 = boş, 1 = dolu

// Disk Düzeni (LBA Adresleri)
#define LBA_SUPERBLOCK 0
#define LBA_INODES_START 1
#define LBA_INODES_COUNT 16
#define LBA_BITMAP 17
#define LBA_DATA_START 18

// static struct vfs_dirent dirent; // Kaldirildi: Thread-safe degil
static vfs_node_t *pafs_root_node = 0;

// Bit işlemleri
static void pafs_bitmap_set(int bit) {
    block_bitmap[bit / 8] |= (1 << (bit % 8));
}
static void pafs_bitmap_clear(int bit) {
    block_bitmap[bit / 8] &= ~(1 << (bit % 8));
}
static int pafs_bitmap_test(int bit) {
    return block_bitmap[bit / 8] & (1 << (bit % 8));
}

extern int strcmp(const char *s1, const char *s2);

// Yardımcı: Boş bir inode bul
static int find_free_inode() {
    for (int i = 0; i < PAFS_MAX_INODES; i++) {
        if (inodes[i].type == PAFS_TYPE_FREE) {
            return i;
        }
    }
    return -1;
}

// Yardımcı: Boş bir blok bul ve dolu işaretle
static int alloc_block() {
    for (int i = LBA_DATA_START; i < PAFS_TOTAL_BLOCKS; i++) {
        if (!pafs_bitmap_test(i)) {
            pafs_bitmap_set(i);
            sb->free_blocks--;
            pafs_save_bitmap();
            pafs_save_superblock();
            return i;
        }
    }
    return -1;
}

// Superblock'ı diske kaydet
static void pafs_save_superblock() {
    ata_write_sector(0, LBA_SUPERBLOCK, (unsigned char *)sb);
}

// Inode tablosunun belirli bir kısmını (1 sektör) diske kaydet
static void pafs_save_inode(int ino) {
    int sector_idx = ino / (512 / sizeof(struct pafs_inode));
    ata_write_sector(0, LBA_INODES_START + sector_idx, (unsigned char *)inodes + (sector_idx * 512));
}

// Bitmap'i diske kaydet
static void pafs_save_bitmap() {
    ata_write_sector(0, LBA_BITMAP, block_bitmap);
}

// Tüm Metadata'yı diske kaydet (Açılış/Format için)
static void save_metadata() {
    pafs_save_superblock();
    for (int i = 0; i < LBA_INODES_COUNT; i++) {
        ata_write_sector(0, LBA_INODES_START + i, (unsigned char *)inodes + (i * 512));
    }
    pafs_save_bitmap();
}

// Diski biçimlendir
void pafs_format(void) {
    sb->magic = PAFS_MAGIC;
    sb->total_blocks = PAFS_TOTAL_BLOCKS;
    sb->total_inodes = PAFS_MAX_INODES;
    sb->free_blocks = PAFS_TOTAL_BLOCKS - LBA_DATA_START; 
    sb->free_inodes = PAFS_MAX_INODES;
    
    for (int i = 0; i < PAFS_MAX_INODES; i++) {
        inodes[i].type = PAFS_TYPE_FREE;
        inodes[i].size = 0;
        for (int j = 0; j < 14; j++) inodes[i].blocks[j] = 0;
    }
    for (int i = 0; i < 512; i++) block_bitmap[i] = 0;
    for (int i = 0; i < LBA_DATA_START; i++) {
        pafs_bitmap_set(i);
    }

    // Root dizini oluştur
    int root_idx = find_free_inode();
    inodes[root_idx].type = PAFS_TYPE_DIR;
    inodes[root_idx].size = 0;
    for (int i = 0; i < 14; i++) inodes[root_idx].blocks[i] = 0;
    
    // Root'un ilk bloğunu hemen ayır ve sıfırla (ls çöp görmesin diye)
    int b = alloc_block();
    inodes[root_idx].blocks[0] = b;
    unsigned char clear_data[512];
    memset(clear_data, 0, 512);
    ata_write_sector(0, b, clear_data);
    
    sb->root_inode = root_idx;
    sb->free_inodes--;
    
    save_metadata();
    put_str("[OK] PAFS formatlandi. Kok dizini temiz.\n");
}

// PAFS Başlatma
void pafs_init(void) {
    spin_lock(&pafs_lock);
    sb = (struct pafs_superblock *)kmalloc(512); 
    inodes = (struct pafs_inode *)kmalloc(16 * 512); // 8192 byte
    block_bitmap = (unsigned char *)kmalloc(512);

    ata_read_sector(0, LBA_SUPERBLOCK, (unsigned char *)sb);

    if (sb->magic == PAFS_MAGIC) {
        put_str("[OK] PAFS disk bulundu. Yukleniyor...\n");
        for (int i = 0; i < LBA_INODES_COUNT; i++) {
            ata_read_sector(0, LBA_INODES_START + i, (unsigned char *)inodes + (i * 512));
        }
        ata_read_sector(0, LBA_BITMAP, block_bitmap);
    } else {
        put_str("[INFO] PAFS disk bulunamadi. Formatlaniyor...\n");
        pafs_format();
    }
    spin_unlock(&pafs_lock);
}

int pafs_mkdir(int parent_ino, const char *name) {
    return pafs_create(parent_ino, name, 1);
}

static int pafs_add_entry(int dir_ino, const char *name, int file_ino) {
    struct pafs_inode *dir = &inodes[dir_ino];
    if (dir->type != PAFS_TYPE_DIR) return -1;
    int max_entries_per_block = PAFS_BLOCK_SIZE / sizeof(struct pafs_dir_entry);
    unsigned char block_data[PAFS_BLOCK_SIZE];
    for (int i = 0; i < 14; i++) {
        if (dir->blocks[i] == 0) {
            int b = alloc_block();
            if (b == -1) return -1;
            dir->blocks[i] = b;
            memset(block_data, 0, PAFS_BLOCK_SIZE);
            ata_write_sector(0, b, block_data);
        }
        ata_read_sector(0, dir->blocks[i], block_data);
        struct pafs_dir_entry *entries = (struct pafs_dir_entry *)block_data;
        for (int j = 0; j < max_entries_per_block; j++) {
            if (entries[j].inode == 0) {
                entries[j].inode = file_ino;
                int k = 0;
                while (name[k] != '\0' && k < 27) {
                    entries[j].name[k] = name[k];
                    k++;
                }
                entries[j].name[k] = '\0';
                ata_write_sector(0, dir->blocks[i], block_data);
                dir->size += sizeof(struct pafs_dir_entry);
                pafs_save_inode(dir_ino);
                return 0;
            }
        }
    }
    return -1;
}

static int pafs_find_entry(int dir_ino, const char *name) {
    struct pafs_inode *dir = &inodes[dir_ino];
    if (dir->type != PAFS_TYPE_DIR || dir->size == 0) return -1;
    int max_entries_per_block = PAFS_BLOCK_SIZE / sizeof(struct pafs_dir_entry);
    unsigned char block_data[PAFS_BLOCK_SIZE];
    for (int i = 0; i < 14; i++) {
        if (dir->blocks[i] == 0) break;
        ata_read_sector(0, dir->blocks[i], block_data);
        struct pafs_dir_entry *entries = (struct pafs_dir_entry *)block_data;
        for (int j = 0; j < max_entries_per_block; j++) {
            if (entries[j].inode != 0 && strcmp(entries[j].name, name) == 0) {
                return entries[j].inode;
            }
        }
    }
    return -1;
}

// Bir dizin boş mu kontrol et ( . ve .. hariç )
int pafs_is_dir_empty(int ino) {
    struct pafs_inode *dir = &inodes[ino];
    if (dir->type != PAFS_TYPE_DIR) return 0;
    return (dir->size == 0); // Şimdilik . ve .. yok, o yüzden 0 ise boştur.
}

// Dizin girişini sil
static int pafs_remove_entry(int dir_ino, const char *name) {
    struct pafs_inode *dir = &inodes[dir_ino];
    int max_entries_per_block = PAFS_BLOCK_SIZE / sizeof(struct pafs_dir_entry);
    
    int target_block = -1;
    int target_idx = -1;
    unsigned char target_data[PAFS_BLOCK_SIZE];

    // 1. Hedef girdiyi bul
    for (int i = 0; i < 14; i++) {
        if (dir->blocks[i] == 0) break;
        ata_read_sector(0, dir->blocks[i], target_data);
        struct pafs_dir_entry *entries = (struct pafs_dir_entry *)target_data;
        for (int j = 0; j < max_entries_per_block; j++) {
            if (entries[j].inode != 0 && strcmp(entries[j].name, name) == 0) {
                target_block = i;
                target_idx = j;
                goto found;
            }
        }
    }
    return -1;

found:
    // 2. Son girdiyi bul (Süreklilik için swap yapacağız)
    int last_entry_total_idx = (dir->size / sizeof(struct pafs_dir_entry)) - 1;
    int last_block_idx = last_entry_total_idx / max_entries_per_block;
    int last_entry_in_block = last_entry_total_idx % max_entries_per_block;
    
    unsigned char last_block_data[PAFS_BLOCK_SIZE];
    ata_read_sector(0, dir->blocks[last_block_idx], last_block_data);
    struct pafs_dir_entry *last_entries = (struct pafs_dir_entry *)last_block_data;
    
    // 3. Swap yap (Eğer hedef son girdi değilse)
    struct pafs_dir_entry *target_entries = (struct pafs_dir_entry *)target_data;
    if (target_block != last_block_idx || target_idx != last_entry_in_block) {
        target_entries[target_idx] = last_entries[last_entry_in_block];
        ata_write_sector(0, dir->blocks[target_block], target_data);
    }
    
    // 4. Son girdiyi sıfırla
    memset(&last_entries[last_entry_in_block], 0, sizeof(struct pafs_dir_entry));
    ata_write_sector(0, dir->blocks[last_block_idx], last_block_data);
    
    // 5. Boyutu güncelle
    dir->size -= sizeof(struct pafs_dir_entry);
    pafs_save_inode(dir_ino);
    return 0;
}

int pafs_delete(int parent_ino, const char *name) {
    spin_lock(&pafs_lock);
    int ino = pafs_find_entry(parent_ino, name);
    if (ino == -1) {
        spin_unlock(&pafs_lock);
        return -1;
    }
    
    struct pafs_inode *file = &inodes[ino];
    
    // Klasör ise boş mu kontrol et
    if (file->type == PAFS_TYPE_DIR && !pafs_is_dir_empty(ino)) {
        spin_unlock(&pafs_lock);
        return -2; // Dizin dolu hatası
    }
    
    // 1. Blokları serbest bırak
    for (int i = 0; i < 14; i++) {
        if (file->blocks[i] != 0) {
            pafs_bitmap_clear(file->blocks[i]);
            sb->free_blocks++;
            file->blocks[i] = 0;
        }
    }
    
    // 2. Inode'u serbest bırak
    file->type = PAFS_TYPE_FREE;
    file->size = 0;
    sb->free_inodes++;
    
    // 3. Ebeveyn dizinden kaldır
    int res = pafs_remove_entry(parent_ino, name);
    pafs_save_superblock();
    pafs_save_bitmap();
    pafs_save_inode(ino);
    spin_unlock(&pafs_lock);
    return res;
}

int pafs_create(int parent_ino, const char *name, int is_dir) {
    spin_lock(&pafs_lock);
    if (sb->free_inodes == 0) {
        spin_unlock(&pafs_lock);
        return -1;
    }
    int dir_ino = parent_ino;
    if (pafs_find_entry(dir_ino, name) != -1) {
        spin_unlock(&pafs_lock);
        return -1;
    }
    int new_ino = find_free_inode();
    if (new_ino == -1) {
        spin_unlock(&pafs_lock);
        return -1;
    }
    inodes[new_ino].type = is_dir ? PAFS_TYPE_DIR : PAFS_TYPE_FILE;
    inodes[new_ino].size = 0;
    for(int i=0; i<14; i++) inodes[new_ino].blocks[i] = 0;
    sb->free_inodes--;
    if (pafs_add_entry(dir_ino, name, new_ino) == 0) {
        spin_unlock(&pafs_lock);
        return new_ino;
    }
    inodes[new_ino].type = PAFS_TYPE_FREE;
    sb->free_inodes++;
    spin_unlock(&pafs_lock);
    return -1;
}

int pafs_write(const char *name, const char *data, int len) {
    spin_lock(&pafs_lock);
    int ino = pafs_find_entry(sb->root_inode, name);
    if (ino == -1) {
        ino = pafs_create(sb->root_inode, name, 0);
        if (ino == -1) {
            spin_unlock(&pafs_lock);
            return -1;
        }
    }
    struct pafs_inode *file = &inodes[ino];
    if (file->type != PAFS_TYPE_FILE) {
        spin_unlock(&pafs_lock);
        return -1;
    }
    int bytes_written = 0;
    int block_idx = 0;
    unsigned char temp_buf[PAFS_BLOCK_SIZE];
    while (bytes_written < len && block_idx < 14) {
        if (file->blocks[block_idx] == 0) {
            int b = alloc_block();
            if (b == -1) break;
            file->blocks[block_idx] = b;
        }
        memset(temp_buf, 0, PAFS_BLOCK_SIZE);
        int to_write = len - bytes_written;
        if (to_write > PAFS_BLOCK_SIZE) to_write = PAFS_BLOCK_SIZE;
        memcpy(temp_buf, data + bytes_written, to_write);
        ata_write_sector(0, file->blocks[block_idx], temp_buf);
        bytes_written += to_write;
        block_idx++;
    }
    file->size = bytes_written;
    pafs_save_inode(ino);
    spin_unlock(&pafs_lock);
    return bytes_written;
}

int pafs_read(const char *name, char *buffer, int max_len) {
    spin_lock(&pafs_lock);
    int ino = pafs_find_entry(sb->root_inode, name);
    if (ino == -1) {
        spin_unlock(&pafs_lock);
        return -1;
    }
    struct pafs_inode *file = &inodes[ino];
    if (file->type != PAFS_TYPE_FILE) {
        spin_unlock(&pafs_lock);
        return -1;
    }
    int total_to_read = file->size;
    if (total_to_read > max_len) total_to_read = max_len;
    int bytes_read = 0;
    int block_idx = 0;
    unsigned char temp_buf[PAFS_BLOCK_SIZE];
    while (bytes_read < total_to_read && block_idx < 14) {
        if (file->blocks[block_idx] == 0) break;
        ata_read_sector(0, file->blocks[block_idx], temp_buf);
        int to_copy = total_to_read - bytes_read;
        if (to_copy > PAFS_BLOCK_SIZE) to_copy = PAFS_BLOCK_SIZE;
        memcpy(buffer + bytes_read, temp_buf, to_copy);
        bytes_read += to_copy;
        block_idx++;
    }
    if (bytes_read < max_len) buffer[bytes_read] = '\0';
    spin_unlock(&pafs_lock);
    return bytes_read;
}

static unsigned int pafs_vfs_read(vfs_node_t *node, unsigned int offset, unsigned int size, unsigned char *buffer) {
    spin_lock(&pafs_lock);
    struct pafs_inode *file = &inodes[node->inode];
    if (file->type != PAFS_TYPE_FILE || file->size == 0) {
        spin_unlock(&pafs_lock);
        return 0;
    }
    if (offset >= file->size) {
        spin_unlock(&pafs_lock);
        return 0;
    }
    if (offset + size > file->size) size = file->size - offset;
    int bytes_read = 0;
    int block_idx = offset / PAFS_BLOCK_SIZE;
    int block_offset = offset % PAFS_BLOCK_SIZE;
    unsigned char temp_buf[PAFS_BLOCK_SIZE];
    while (bytes_read < size && block_idx < 14) {
        ata_read_sector(0, file->blocks[block_idx], temp_buf);
        int to_copy = PAFS_BLOCK_SIZE - block_offset;
        if (to_copy > (size - bytes_read)) to_copy = size - bytes_read;
        memcpy(buffer + bytes_read, temp_buf + block_offset, to_copy);
        bytes_read += to_copy;
        block_idx++;
        block_offset = 0;
    }
    spin_unlock(&pafs_lock);
    return bytes_read;
}

static unsigned int pafs_vfs_write(vfs_node_t *node, unsigned int offset, unsigned int size, unsigned char *buffer) {
    spin_lock(&pafs_lock);
    struct pafs_inode *file = &inodes[node->inode];
    if (file->type != PAFS_TYPE_FILE) {
        spin_unlock(&pafs_lock);
        return 0;
    }
    
    // PAFS sadece append modunu (veya tam blok baştan yazmayı) tam destekler. 
    // Basitlik adına, doğrudan dosyanın üzerine yazacağız. 
    // Eğer offset 0 ise ve size mevcut dosya boyutundan farklıysa boyut değişebilir.
    int bytes_written = 0;
    int block_idx = offset / PAFS_BLOCK_SIZE;
    int block_offset = offset % PAFS_BLOCK_SIZE;
    unsigned char temp_buf[PAFS_BLOCK_SIZE];
    
    while (bytes_written < size && block_idx < 14) {
        if (file->blocks[block_idx] == 0) {
            int b = alloc_block();
            if (b == -1) break;
            file->blocks[block_idx] = b;
        }
        
        // Eğer bloğun bir kısmına yazıyorsak, önce eski veriyi okumalıyız
        if (block_offset > 0 || (size - bytes_written) < PAFS_BLOCK_SIZE) {
            ata_read_sector(0, file->blocks[block_idx], temp_buf);
        } else {
            memset(temp_buf, 0, PAFS_BLOCK_SIZE);
        }
        
        int to_write = PAFS_BLOCK_SIZE - block_offset;
        if (to_write > (size - bytes_written)) to_write = size - bytes_written;
        
        memcpy(temp_buf + block_offset, buffer + bytes_written, to_write);
        ata_write_sector(0, file->blocks[block_idx], temp_buf);
        
        bytes_written += to_write;
        block_idx++;
        block_offset = 0;
    }
    
    // Boyut büyüdüyse güncelle
    if (offset + bytes_written > file->size) {
        file->size = offset + bytes_written;
    }
    
    pafs_save_inode(node->inode);
    spin_unlock(&pafs_lock);
    return bytes_written;
}

static void pafs_vfs_close(vfs_node_t *node) {
    if (node && node != pafs_root_node) {
        if (node->impl != 0) {
            kfree((void *)node->impl);
        }
        kfree(node);
    }
}

static struct vfs_dirent *pafs_vfs_readdir(vfs_node_t *node, unsigned int index) {
    // Statik dirent yerine heap veya stack kullanmaliyiz. 
    // Ancak readdir arayüzü pointer döndüğü için düğüm içinde bir tampon tutmak en temizi.
    // Şimdilik düğümün impl alanını veya ek bir alanı kullanabiliriz.
    // vfs_dirent 132 byte. vfs_node_t içinde yer yok. 
    // Bu yüzden geçici bir dirent allocate edip dönüyoruz (veya düğümün sonuna ekliyoruz).
    // Basitlik için düğümün 'impl' kısmını bir dirent pointer'ı olarak kullanalım.

    if (node->impl == 0) {
        node->impl = (unsigned int)kmalloc(sizeof(struct vfs_dirent));
    }
    struct vfs_dirent *d = (struct vfs_dirent *)node->impl;
    memset(d, 0, sizeof(struct vfs_dirent));

    spin_lock(&pafs_lock);
    struct pafs_inode *dir = &inodes[node->inode];
    if (dir->type != PAFS_TYPE_DIR || dir->size == 0) {
        spin_unlock(&pafs_lock);
        return 0;
    }
    int max_entries_per_block = PAFS_BLOCK_SIZE / sizeof(struct pafs_dir_entry);
    int block_idx = index / max_entries_per_block;
    int entry_in_block = index % max_entries_per_block;
    int total_entries = dir->size / sizeof(struct pafs_dir_entry);
    if (index >= total_entries) {
        spin_unlock(&pafs_lock);
        return 0;
    }
    if (block_idx >= 14 || dir->blocks[block_idx] == 0) {
        spin_unlock(&pafs_lock);
        return 0;
    }
    unsigned char dir_data[PAFS_BLOCK_SIZE];
    ata_read_sector(0, dir->blocks[block_idx], dir_data);
    struct pafs_dir_entry *entries = (struct pafs_dir_entry *)dir_data;
    d->ino = entries[entry_in_block].inode;
    int i = 0;
    while (entries[entry_in_block].name[i] != '\0' && i < 27) {
        d->name[i] = entries[entry_in_block].name[i];
        i++;
    }
    d->name[i] = '\0';
    spin_unlock(&pafs_lock);
    return d;
}

static vfs_node_t *pafs_vfs_finddir(vfs_node_t *node, char *name) {
    spin_lock(&pafs_lock);
    int ino = pafs_find_entry(node->inode, name);
    if (ino != -1) {
        vfs_node_t *res = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
        memset(res, 0, sizeof(vfs_node_t));
        struct pafs_inode *target_inode = &inodes[ino];
        int j = 0;
        while (name[j] != '\0' && j < 127) {
            res->name[j] = name[j];
            j++;
        }
        res->name[j] = '\0';
        res->inode = ino;
        res->length = target_inode->size;
        res->flags = (target_inode->type == PAFS_TYPE_DIR) ? VFS_DIRECTORY : VFS_FILE;
        res->read = pafs_vfs_read;
        res->write = pafs_vfs_write;
        res->readdir = pafs_vfs_readdir;
        res->finddir = pafs_vfs_finddir;
        res->close = pafs_vfs_close; // Close fonksiyonunu ata
        spin_unlock(&pafs_lock);
        return res;
    }
    spin_unlock(&pafs_lock);
    return 0;
}

vfs_node_t *pafs_get_vfs_root() {
    if (pafs_root_node) return pafs_root_node;
    pafs_root_node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    memset(pafs_root_node, 0, sizeof(vfs_node_t));
    pafs_root_node->name[0] = '/';
    pafs_root_node->name[1] = '\0';
    pafs_root_node->inode = sb->root_inode;
    pafs_root_node->flags = VFS_DIRECTORY;
    pafs_root_node->length = inodes[sb->root_inode].size;
    pafs_root_node->readdir = pafs_vfs_readdir;
    pafs_root_node->finddir = pafs_vfs_finddir;
    pafs_root_node->close = pafs_vfs_close; // Klonlanan root düğümlerin temizlenmesi için close atanıyor
    return pafs_root_node;
}
