#include "idt.h"
#include "task.h"

// Port okuma/yazma yardımcı fonksiyonları (Eğer başka yerde tanımlamadıysan buraya ekle)
extern void outb(unsigned short port, unsigned char val);
extern unsigned char inb(unsigned short port);
extern void put_str(const char* str);

// Donanım işleyicileri için fonksiyon işaretçisi dizisi
void *irq_routines[16] = { 0 };

// Bir donanım için işleyici (handler) kaydeder
void irq_install_handler(int irq, void (*handler)(struct registers *r)) {
    irq_routines[irq] = handler;
}

// Bir IRQ işleyicisini siler
void irq_uninstall_handler(int irq) {
    irq_routines[irq] = 0;
}

// PIC Yeniden Haritalama (Remapping)
// IRQ 0-7 -> IDT 32-39 | IRQ 8-15 -> IDT 40-47
void irq_remap(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    
    // Maskeleme: TUM KESMELER KAPALI (Hata ayıklama için)
    outb(0x21, 0xFF); 
    outb(0xA1, 0xFF); 
}

// IDT'den IRQ'ları çağıran ana C fonksiyonu
unsigned int irq_handler(struct registers *r) {
    // Boş bir fonksiyon işaretçisi tanımlayalım
    void (*handler)(struct registers *r);

    // Bu IRQ için kayıtlı özel bir fonksiyon var mı? (Sınır kontrolü ekledik)
    if (r->int_no >= 32 && r->int_no < 48) {
        handler = irq_routines[r->int_no - 32];
        if (handler) {
            handler(r);
        }
    }

    // PIC'e "End of Interrupt" (EOI) sinyali gönder
    // Eğer IRQ Slave PIC'ten (8-15) geldiyse ona da gönder
    if (r->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);

    // Eğer PIT (Zamanlayıcı) kesmesi ise ve multitasking aktifse görev değiştir
    if (r->int_no == 32) {
        return schedule_internal((unsigned int)r);
    }

    return (unsigned int)r;
}