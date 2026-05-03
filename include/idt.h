#ifndef IDT_H
#define IDT_H

#include "common.h"

// IDT Entry Yapısı
struct idt_entry_struct {
    unsigned short base_low;
    unsigned short sel;
    unsigned char  always0;
    unsigned char  flags;
    unsigned short base_high;
} __attribute__((packed));

// IDT Pointer Yapısı
struct idt_ptr_struct {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));

// Fonksiyonlar
void init_gdt();
void init_idt();
void irq_remap();
void irq_install_handler(int irq, void (*handler)(struct registers *r));
void init_keyboard();

// Global Değişkenler
extern struct idt_entry_struct idt_entries[256];

// Assembly'den gelenler
extern void isr0();
extern void isr14();
extern void irq0();
extern void irq1();

#endif