#include "pafs.h"
#include "kheap.h"
#include "ata.h"

// Bellek içi hızlı erişim işaretçileri
static struct pafs_superblock *sb;
static struct pafs_inode *inodes;
static unsigned char *block_bitmap; // 0 = boş, 1 = dolu

// Disk Düzeni (LBA Adresleri)
#define LBA_SUPERBLOCK 0
#define LBA_INODES_START 1
#define LBA_INODES_COUNT 14
#define LBA_BITMAP 15
#define LBA_DATA_START 16

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
    for (int i = 0; i < PAFS_TOTAL_BLOCKS; i++) {
        if (!pafs_bitmap_test(i)) {
            pafs_bitmap_set(i);
            sb->free_blocks--;
            return i;
        }
    }
    return -1;
}

// PAFS Başlatma (Belleği ayır ve diskten oku)
void pafs_init(void) {
    sb = (struct pafs_superblock *)kmalloc(512); 
    inodes = (struct pafs_inode *)kmalloc(PAFS_MAX_INODES * sizeof(struct pafs_inode));
    block_bitmap = (unsigned char *)kmalloc(512); // 4096 bit = 512 byte (1 sektör)

    // Diskin ilk sektörünü (Superblock) oku
    ata_read_sector(LBA_SUPERBLOCK, (unsigned char *)sb);

    if (sb->magic == PAFS_MAGIC) {
        put_str("[OK] PAFS disk bulundu. Yukleniyor...\n");
        // Inode'ları oku
        for (int i = 0; i < LBA_INODES_COUNT; i++) {
            ata_read_sector(LBA_INODES_START + i, (unsigned char *)inodes + (i * 512));
        }
        // Bitmap'i oku
        ata_read_sector(LBA_BITMAP, block_bitmap);
    } else {
        put_str("[INFO] PAFS disk bulunamadi. Formatlaniyor...\n");
        pafs_format();
    }
}

// Metadata'yı diske kaydet
static void save_metadata() {
    ata_write_sector(LBA_SUPERBLOCK, (unsigned char *)sb);
    for (int i = 0; i < LBA_INODES_COUNT; i++) {
        ata_write_sector(LBA_INODES_START + i, (unsigned char *)inodes + (i * 512));
    }
    ata_write_sector(LBA_BITMAP, block_bitmap);
}

// Diski biçimlendir
void pafs_format(void) {
    sb->magic = PAFS_MAGIC;
    sb->total_blocks = PAFS_TOTAL_BLOCKS;
    sb->total_inodes = PAFS_MAX_INODES;
    sb->free_blocks = PAFS_TOTAL_BLOCKS - LBA_DATA_START; // İlk 16 blok metadata
    sb->free_inodes = PAFS_MAX_INODES;
    
    for (int i = 0; i < PAFS_MAX_INODES; i++) {
        inodes[i].type = PAFS_TYPE_FREE;
        inodes[i].size = 0;
    }
    // Bitmap temizle ve metadata bloklarını işaretle
    for (int i = 0; i < 512; i++) block_bitmap[i] = 0;
    for (int i = 0; i < LBA_DATA_START; i++) {
        pafs_bitmap_set(i);
    }

    int root_idx = find_free_inode();
    inodes[root_idx].type = PAFS_TYPE_DIR;
    inodes[root_idx].size = 0;
    sb->root_inode = root_idx;
    sb->free_inodes--;

    save_metadata();
    put_str("[OK] PAFS formatlandi. Kok dizini (/) hazir.\n");
}

// Dosya oluştur
int pafs_create(const char *name, int is_dir) {
    if (sb->free_inodes == 0) return -1;
    
    // İsim uzunluk kontrolü (basit tutalım, max 27)
    
    // Root dizinine (Inode 0) yeni dosyayı eklemeliyiz.
    struct pafs_inode *root = &inodes[sb->root_inode];
    
    // Root'un data bloğu var mı?
    if (root->size == 0) {
        int b = alloc_block();
        if (b == -1) return -1;
        root->blocks[0] = b;
    }
    
    // Aynı isimde dosya var mı kontrolü (Basitlik için şimdilik atlıyoruz)
    
    // Yeni inode ayır
    int new_ino = find_free_inode();
    if (new_ino == -1) return -1;
    
    inodes[new_ino].type = is_dir ? PAFS_TYPE_DIR : PAFS_TYPE_FILE;
    inodes[new_ino].size = 0;
    sb->free_inodes--;
    
    // Root dizinine Dir Entry ekle
    unsigned char root_data[512];
    ata_read_sector(root->blocks[0], root_data);
    struct pafs_dir_entry *entries = (struct pafs_dir_entry *)root_data;
    
    int max_entries = PAFS_BLOCK_SIZE / sizeof(struct pafs_dir_entry);
    int entry_count = root->size / sizeof(struct pafs_dir_entry);
    
    if (entry_count < max_entries) {
        entries[entry_count].inode = new_ino;
        int i = 0;
        while (name[i] != '\0' && i < 27) {
            entries[entry_count].name[i] = name[i];
            i++;
        }
        entries[entry_count].name[i] = '\0';
        
        root->size += sizeof(struct pafs_dir_entry);
        
        // Root datayı ve metadatayı diske yaz
        ata_write_sector(root->blocks[0], root_data);
        save_metadata();
        
        return new_ino;
    }
    return -1; // Root dizini dolu
}

// Dosyaya yaz
int pafs_write(const char *name, const char *data, int len) {
    // 1. Dosyayı bul
    struct pafs_inode *root = &inodes[sb->root_inode];
    if (root->size == 0) return -1;
    
    unsigned char root_data[512];
    ata_read_sector(root->blocks[0], root_data);
    struct pafs_dir_entry *entries = (struct pafs_dir_entry *)root_data;
    int entry_count = root->size / sizeof(struct pafs_dir_entry);
    
    int target_ino = -1;
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            target_ino = entries[i].inode;
            break;
        }
    }
    
    if (target_ino == -1 || inodes[target_ino].type != PAFS_TYPE_FILE) return -1;
    
    struct pafs_inode *file = &inodes[target_ino];
    
    if (len > PAFS_BLOCK_SIZE) len = PAFS_BLOCK_SIZE;
    
    if (file->size == 0) {
        int b = alloc_block();
        if (b == -1) return -1;
        file->blocks[0] = b;
    }
    
    unsigned char file_data[512];
    for (int i = 0; i < 512; i++) file_data[i] = 0; // Temizle
    
    for (int i = 0; i < len; i++) {
        file_data[i] = data[i];
    }
    file->size = len;
    
    ata_write_sector(file->blocks[0], file_data);
    save_metadata();
    
    return len;
}

// Dosyadan oku
int pafs_read(const char *name, char *buffer, int max_len) {
    // 1. Dosyayı bul
    struct pafs_inode *root = &inodes[sb->root_inode];
    if (root->size == 0) return -1;
    
    unsigned char root_data[512];
    ata_read_sector(root->blocks[0], root_data);
    struct pafs_dir_entry *entries = (struct pafs_dir_entry *)root_data;
    int entry_count = root->size / sizeof(struct pafs_dir_entry);
    
    int target_ino = -1;
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            target_ino = entries[i].inode;
            break;
        }
    }
    
    if (target_ino == -1 || inodes[target_ino].type != PAFS_TYPE_FILE) return -1;
    
    struct pafs_inode *file = &inodes[target_ino];
    if (file->size == 0) return 0;
    
    int read_len = file->size;
    if (read_len > max_len) read_len = max_len;
    
    unsigned char file_data[512];
    ata_read_sector(file->blocks[0], file_data);
    
    for (int i = 0; i < read_len; i++) {
        buffer[i] = file_data[i];
    }
    buffer[read_len] = '\0'; 
    
    return read_len;
}

// Dizini Listele (ls)
void pafs_list_dir(void) {
    struct pafs_inode *root = &inodes[sb->root_inode];
    if (root->size == 0) {
        put_str("Dizin bos.\n");
        return;
    }
    
    unsigned char root_data[512];
    ata_read_sector(root->blocks[0], root_data);
    struct pafs_dir_entry *entries = (struct pafs_dir_entry *)root_data;
    int entry_count = root->size / sizeof(struct pafs_dir_entry);
    
    put_str("PAFS Kok Dizini (/):\n");
    for (int i = 0; i < entry_count; i++) {
        struct pafs_inode *fnode = &inodes[entries[i].inode];
        if (fnode->type == PAFS_TYPE_DIR) {
            put_str("[DIR]  ");
        } else {
            put_str("[FILE] ");
        }
        put_str(entries[i].name);
        put_str("  (Boyut: ");
        put_int(fnode->size);
        put_str(" byte)\n");
    }
}
