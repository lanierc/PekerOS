#ifndef KHEAP_H
#define KHEAP_H

#include "common.h"

// Heap blok başlığı
struct heap_header {
    unsigned int size;           // Header + Veri boyutu
    unsigned int is_free;        // 1 = Boş, 0 = Dolu
    struct heap_header *next;    // Sonraki blok
    struct heap_header *prev;    // Önceki blok
};

// Fonksiyon prototipleri
void init_kheap(void);
void *kmalloc(unsigned int size);
void kfree(void *ptr);

#endif
