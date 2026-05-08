#include "idt.h"

struct idt_entry_struct idt_entries[256] __attribute__((aligned(16)));

// Assembly'deki pointer yapısı
extern struct idt_ptr_struct idt_ptr_asm;

extern void idt_flush();
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
extern void int80_handler();
extern void isr0();
extern void isr14();

void idt_set_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

// ISR Handler - CPU istisnaları için
void isr_handler(struct registers *regs) {
    if (regs->int_no == 14) {
        unsigned int cr2;
        asm volatile("mov %%cr2, %0" : "=r" (cr2));
        panic("PAGE FAULT", regs);
    } else {
        // Basit bir mesaj oluşturma (sprintf olmadığı için kısıtlı)
        if (regs->int_no == 0) panic("DIVISION BY ZERO", regs);
        else if (regs->int_no == 13) panic("GENERAL PROTECTION FAULT", regs);
        else if (regs->int_no == 8) panic("DOUBLE FAULT", regs);
        else panic("BILINMEYEN CPU ISTISNASI", regs);
    }

    // Buraya asla ulaşılmamalı
    asm volatile("cli");
    for(;;) asm volatile("hlt");
}

void init_idt() {
    idt_ptr_asm.limit = sizeof(struct idt_entry_struct) * 256 - 1;
    idt_ptr_asm.base  = (unsigned int)&idt_entries;

    // Tüm IDT girişlerini isr0 ile doldur (varsayılan yakalayıcı)
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, (unsigned int)isr0, 0x08, 0x8E);
    }

    // Page Fault (Exception 14) için özel isr
    idt_set_gate(14, (unsigned int)isr14, 0x08, 0x8E);

    // IRQ'lar için özel işleyiciler (PIC remap sonrası IDT 32-47)
    idt_set_gate(32, (unsigned int)irq0, 0x08, 0x8E);
    idt_set_gate(33, (unsigned int)irq1, 0x08, 0x8E);
    idt_set_gate(34, (unsigned int)irq2, 0x08, 0x8E);
    idt_set_gate(35, (unsigned int)irq3, 0x08, 0x8E);
    idt_set_gate(36, (unsigned int)irq4, 0x08, 0x8E);
    idt_set_gate(37, (unsigned int)irq5, 0x08, 0x8E);
    idt_set_gate(38, (unsigned int)irq6, 0x08, 0x8E);
    idt_set_gate(39, (unsigned int)irq7, 0x08, 0x8E);
    idt_set_gate(40, (unsigned int)irq8, 0x08, 0x8E);
    idt_set_gate(41, (unsigned int)irq9, 0x08, 0x8E);
    idt_set_gate(42, (unsigned int)irq10, 0x08, 0x8E);
    idt_set_gate(43, (unsigned int)irq11, 0x08, 0x8E);
    idt_set_gate(44, (unsigned int)irq12, 0x08, 0x8E);
    idt_set_gate(45, (unsigned int)irq13, 0x08, 0x8E);
    idt_set_gate(46, (unsigned int)irq14, 0x08, 0x8E);
    idt_set_gate(47, (unsigned int)irq15, 0x08, 0x8E);
    
    // Sistem Cagrisi (int 0x80) - DPL=3 (Kullanıcı çağırabilir)
    idt_set_gate(128, (unsigned int)int80_handler, 0x08, 0xEE);

    idt_flush();
}