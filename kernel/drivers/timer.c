#include "idt.h"
#include "timer.h"

// PIT sabitler
#define PIT_CMD  0x43
#define PIT_CH0  0x40
#define PIT_FREQ 1193182  // PIT temel frekansı (Hz)

// Dahili sayaçlar
static volatile unsigned int tick_count = 0;
static unsigned int timer_freq = 0;

// IRQ0 Handler - Her tick'te çağrılır
static void timer_handler(struct registers *r) {
    (void)r;  // Kullanılmıyor
    tick_count++;
}

// PIT'i belirli bir frekansta başlat
void init_timer(unsigned int frequency) {
    timer_freq = frequency;
    tick_count = 0;

    // PIT bölücüsünü hesapla
    unsigned int divisor = PIT_FREQ / frequency;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1) divisor = 1;

    // PIT Komutu: Channel 0, lobyte/hibyte, Mode 3 (kare dalga), binary
    outb(PIT_CMD, 0x36);

    // Bölücüyü gönder (önce düşük byte, sonra yüksek byte)
    outb(PIT_CH0, (unsigned char)(divisor & 0xFF));
    outb(PIT_CH0, (unsigned char)((divisor >> 8) & 0xFF));

    // Timer handler'ını kaydet (IRQ0)
    irq_install_handler(0, timer_handler);

    // IRQ0'ı (timer) aç - Master PIC maskeleme kaydını güncelle
    unsigned char mask = inb(0x21);
    mask &= ~(1 << 0);  // Bit 0'ı temizle = IRQ0 açık
    outb(0x21, mask);
}

// Toplam tick sayısını döndür
unsigned int timer_get_ticks() {
    return tick_count;
}

// Geçen süreyi saniye olarak döndür
unsigned int timer_get_seconds() {
    if (timer_freq == 0) return 0;
    return tick_count / timer_freq;
}

// Belirli milisaniye kadar bekle
void sleep(unsigned int ms) {
    if (timer_freq == 0) return;

    // ms'yi tick sayısına çevir
    unsigned int ticks_to_wait = (ms * timer_freq) / 1000;
    if (ticks_to_wait == 0) ticks_to_wait = 1;

    unsigned int start = tick_count;
    while ((tick_count - start) < ticks_to_wait) {
        // sti;hlt atomik çifti: kesmeleri aç ve hemen uyu
        // (IRQ handler içinden çağrıldığında IF=0 olduğu için sti şart)
        asm volatile("sti; hlt");
    }
}
