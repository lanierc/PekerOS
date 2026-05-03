#ifndef TSS_H
#define TSS_H

#include "common.h"

struct tss_entry_struct {
    unsigned int prev_tss;
    unsigned int esp0;
    unsigned int ss0;
    unsigned int esp1, ss1, esp2, ss2;
    unsigned int cr3;
    unsigned int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    unsigned int es, cs, ss, ds, fs, gs;
    unsigned int ldt;
    unsigned short trap, iomap_base;
} __attribute__((packed));

void write_tss(int num, unsigned short ss0, unsigned int esp0);
void set_kernel_stack(unsigned int stack);

#endif
