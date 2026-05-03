#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "idt.h"

#define PAGE_SIZE 4096

// Page Table Entry (PTE)
struct page_table_entry {
    unsigned int present    : 1;   // Sayfa bellekte mi?
    unsigned int rw         : 1;   // 0 = Sadece Okuma, 1 = Okuma/Yazma
    unsigned int user       : 1;   // 0 = Supervisor (Kernel), 1 = User
    unsigned int accessed   : 1;   // Okundu/Yazıldı mı?
    unsigned int dirty      : 1;   // Değiştirildi mi?
    unsigned int unused     : 7;   // Kullanılmayan bitler
    unsigned int frame      : 20;  // Fiziksel Frame Adresi (sayfa numarası)
};

// Page Directory Entry (PDE)
struct page_dir_entry {
    unsigned int present    : 1;   // Page Table bellekte mi?
    unsigned int rw         : 1;   // Okuma/Yazma izni
    unsigned int user       : 1;   // User/Supervisor
    unsigned int write_thru : 1;   // Write-Through Caching
    unsigned int cache_dis  : 1;   // Cache Disable
    unsigned int accessed   : 1;   // Okundu/Yazıldı mı?
    unsigned int reserved   : 1;   // Ayrılmış (0)
    unsigned int page_size  : 1;   // 0 = 4KB sayfa, 1 = 4MB sayfa
    unsigned int ignored    : 4;   // Kullanılmayan
    unsigned int pt_frame   : 20;  // Page Table'ın Frame Adresi
};

// Page Table
struct page_table {
    struct page_table_entry entries[1024];
};

// Page Directory
struct page_directory {
    struct page_dir_entry entries[1024];
    // Gelişmiş sistemlerde fiziksel/sanal tabloları tutmak için eklenebilir
};

// Paging Fonksiyonları
void init_paging(void);
void page_fault(struct registers *r);
void paging_map_memory(unsigned int phys, unsigned int virt, unsigned int size);

#endif
