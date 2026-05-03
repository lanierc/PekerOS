#include "kheap.h"

// 4 MB'lık statik Heap alanı (.bss bölümüne hizalı olarak yerleşecek)
#define HEAP_SIZE (4 * 1024 * 1024)
static unsigned char heap_space[HEAP_SIZE] __attribute__((aligned(4096)));

static struct heap_header *heap_first;

void init_kheap(void) {
    // Tüm alanı tek bir büyük boş blok olarak başlat
    heap_first = (struct heap_header *)heap_space;
    heap_first->size = HEAP_SIZE;
    heap_first->is_free = 1;
    heap_first->next = 0;
    heap_first->prev = 0;

    put_str("[OK] Kernel Heap (Dinamik Bellek) baslatildi (4MB).\n");
}

void *kmalloc(unsigned int size) {
    if (size == 0) return 0;

    // Her blok 4 byte hizalı olmalı
    unsigned int aligned_size = size;
    if (aligned_size % 4 != 0) {
        aligned_size += 4 - (aligned_size % 4);
    }

    unsigned int total_size = aligned_size + sizeof(struct heap_header);

    struct heap_header *current = heap_first;
    
    // İlk uygun bloğu bul (First-Fit)
    while (current != 0) {
        if (current->is_free && current->size >= total_size) {
            
            // Eğer blok gereğinden çok büyükse, bloğu ikiye böl (Split)
            // Kalan alanın yeni bir header alabilecek kadar büyük olması gerekir
            if (current->size > total_size + sizeof(struct heap_header) + 4) {
                struct heap_header *new_block = (struct heap_header *)((unsigned int)current + total_size);
                
                new_block->size = current->size - total_size;
                new_block->is_free = 1;
                new_block->prev = current;
                new_block->next = current->next;
                
                if (current->next != 0) {
                    current->next->prev = new_block;
                }
                
                current->next = new_block;
                current->size = total_size;
            }

            // Bloğu dolu olarak işaretle ve verinin başlangıç adresini dön
            current->is_free = 0;
            return (void *)((unsigned int)current + sizeof(struct heap_header));
        }
        current = current->next;
    }

    put_str("[HATA] Kernel Heap yetersiz bellek!\n");
    return 0; // Yer bulunamadı
}

void kfree(void *ptr) {
    if (ptr == 0) return;

    // Veri adresinden geriye doğru giderek header'ı bul
    struct heap_header *block = (struct heap_header *)((unsigned int)ptr - sizeof(struct heap_header));
    
    if (block->is_free) {
        put_str("[HATA] Zaten bos olan bir bellek serbest birakilmak istendi!\n");
        return;
    }

    block->is_free = 1;

    // Sağdaki (Sonraki) blok boşsa birleştir (Merge Right)
    if (block->next != 0 && block->next->is_free) {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next != 0) {
            block->next->prev = block;
        }
    }

    // Soldaki (Önceki) blok boşsa birleştir (Merge Left)
    if (block->prev != 0 && block->prev->is_free) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next != 0) {
            block->next->prev = block->prev;
        }
    }
}
