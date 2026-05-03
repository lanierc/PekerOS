#include "pmm.h"
#include "common.h"

// Maksimum 128MB destekle (32768 frame, 4KB bitmap)
#define MAX_FRAMES 32768

// Bitmap: 1 = kullanımda, 0 = boş
static unsigned char frame_bitmap[MAX_FRAMES / 8];
static unsigned int total_frames = 0;
static unsigned int used_frames_count = 0;
static unsigned int total_memory_kb = 0;

// Linker'dan gelen kernel sınırları
extern unsigned int __kernel_start;
extern unsigned int __kernel_end;

// --- Bitmap İşlemleri ---

static void bitmap_set(unsigned int frame) {
    frame_bitmap[frame / 8] |= (1 << (frame % 8));
}

static void bitmap_clear(unsigned int frame) {
    frame_bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static int bitmap_test(unsigned int frame) {
    return (frame_bitmap[frame / 8] >> (frame % 8)) & 1;
}

// --- PMM Başlatma ---

void init_pmm(struct multiboot_info *mbi) {
    // 1. Tüm frame'leri "kullanımda" olarak işaretle
    for (unsigned int i = 0; i < MAX_FRAMES / 8; i++) {
        frame_bitmap[i] = 0xFF;
    }

    // 2. Toplam belleği hesapla
    if (mbi->flags & MB_FLAG_MEM) {
        // mem_upper: 1MB üstü boş RAM (KB cinsinden)
        total_memory_kb = mbi->mem_lower + mbi->mem_upper + 1024;
    }

    total_frames = total_memory_kb / 4;  // Her frame 4KB
    if (total_frames > MAX_FRAMES) total_frames = MAX_FRAMES;

    // 3. Bellek haritasını kullanarak boş bölgeleri işaretle
    if (mbi->flags & MB_FLAG_MMAP) {
        // mbi->mmap_addr fiziksel adres içerir, sayfalama (paging) açık olduğu için
        // okumak üzere sanal adrese (+0xC0000000) çeviriyoruz.
        unsigned int addr = mbi->mmap_addr + 0xC0000000;
        unsigned int end  = addr + mbi->mmap_length;

        while (addr < end) {
            struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)addr;

            // Sadece 32-bit adres alanındaki kullanılabilir bölgeleri al
            if (entry->type == MMAP_TYPE_AVAILABLE && entry->base_addr_high == 0) {
                unsigned int base   = entry->base_addr_low;
                unsigned int length = entry->length_low;

                // Frame sınırlarına hizala (başı yukarı, sonu aşağı)
                unsigned int frame_start = (base + FRAME_SIZE - 1) / FRAME_SIZE;
                unsigned int frame_end   = (base + length) / FRAME_SIZE;

                for (unsigned int f = frame_start; f < frame_end && f < total_frames; f++) {
                    bitmap_clear(f);
                }
            }

            addr += entry->size + sizeof(entry->size);
        }
    } else {
        // Fallback: mem_upper kullanarak 1MB üstünü serbest bırak
        unsigned int upper_frames = mbi->mem_upper / 4;
        unsigned int start_frame  = 0x100000 / FRAME_SIZE;  // 256. frame = 1MB
        for (unsigned int f = start_frame; f < start_frame + upper_frames && f < total_frames; f++) {
            bitmap_clear(f);
        }
    }

    // 4. İlk 1MB'ı her zaman "kullanımda" işaretle (BIOS, VGA, vb.)
    for (unsigned int f = 0; f < 256; f++) {
        bitmap_set(f);
    }

    // 5. Kernel bölgesini "kullanımda" işaretle
    // Linker'dan gelen adresler sanal (0xC0100000+), PMM için fiziksel adreslere çeviriyoruz
    unsigned int kernel_start_addr = (unsigned int)&__kernel_start - 0xC0000000;
    unsigned int kernel_end_addr   = (unsigned int)&__kernel_end - 0xC0000000;
    unsigned int ks_frame = kernel_start_addr / FRAME_SIZE;
    unsigned int ke_frame = (kernel_end_addr + FRAME_SIZE - 1) / FRAME_SIZE;
    for (unsigned int f = ks_frame; f < ke_frame && f < total_frames; f++) {
        bitmap_set(f);
    }

    // 6. Kullanımdaki frame sayısını hesapla
    used_frames_count = 0;
    for (unsigned int f = 0; f < total_frames; f++) {
        if (bitmap_test(f)) used_frames_count++;
    }
}

// --- Frame Ayırma ---

unsigned int alloc_frame(void) {
    for (unsigned int f = 0; f < total_frames; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames_count++;
            return f * FRAME_SIZE;  // Fiziksel adres döndür
        }
    }
    return 0;  // Bellek yetersiz!
}

// --- Frame Serbest Bırakma ---

void free_frame(unsigned int addr) {
    unsigned int frame = addr / FRAME_SIZE;
    if (frame < total_frames && bitmap_test(frame)) {
        bitmap_clear(frame);
        used_frames_count--;
    }
}

// --- İstatistikler ---

unsigned int pmm_total_memory_kb(void) { return total_memory_kb; }
unsigned int pmm_total_frames(void)    { return total_frames; }
unsigned int pmm_used_frames(void)     { return used_frames_count; }
unsigned int pmm_free_frames(void)     { return total_frames - used_frames_count; }
