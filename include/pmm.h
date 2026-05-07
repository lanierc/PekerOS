#ifndef PMM_H
#define PMM_H

#include "multiboot.h"

#define FRAME_SIZE 4096   // 4KB sayfa boyutu

void init_pmm(struct multiboot_info *mbi);
unsigned int alloc_frame(void);
unsigned int alloc_contiguous_frames(int count);
void free_frame(unsigned int addr);

// Bellek istatistikleri
unsigned int pmm_total_memory_kb(void);
unsigned int pmm_total_frames(void);
unsigned int pmm_used_frames(void);
unsigned int pmm_free_frames(void);

#endif
