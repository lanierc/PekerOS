#include "tss.h"

struct tss_entry_struct tss_entry;

extern void gdt_set_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran);
extern void tss_flush();

void write_tss(int num, unsigned short ss0, unsigned int esp0) {
    unsigned int base = (unsigned int)&tss_entry;
    unsigned int limit = sizeof(tss_entry);

    // GDT'ye TSS girdisini ekle (Access: 0x89)
    gdt_set_gate(num, base, limit, 0x89, 0x40);

    // TSS yapısını temizle
    for (int i = 0; i < sizeof(tss_entry); i++) {
        ((unsigned char*)&tss_entry)[i] = 0;
    }

    tss_entry.ss0 = ss0;   // Kernel Data Segment
    tss_entry.esp0 = esp0; // Kernel Stack Pointer
    
    // IO Map base adresini ayarla (I/O port erişimini kısıtlamak için)
    tss_entry.iomap_base = sizeof(tss_entry);

    tss_flush();
}

void set_kernel_stack(unsigned int stack) {
    tss_entry.esp0 = stack;
}
