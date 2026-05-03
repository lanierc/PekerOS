#include "idt.h"
#include "shell.h"
#include "timer.h"
#include "pmm.h"
#include "paging.h"
#include "kheap.h"
#include "pafs.h"
#include "task.h"
#include "tss.h"
#include "syscall.h"
#include "pci.h"
#include "vbe.h"
#include "mouse.h"

// Global değişkenler
unsigned int terminal_row = 0;
unsigned int terminal_col = 0;
char* const video_memory = (char*) 0xC00B8000;

// Port I/O
void outb(unsigned short port, unsigned char val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void outw(unsigned short port, unsigned short val) {
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

unsigned short inw(unsigned short port) {
    unsigned short ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void vbe_init(struct multiboot_info *mb_info);
void vbe_put_pixel(int x, int y, unsigned int color);
void vbe_clear_screen(unsigned int color);
void vbe_draw_gradient();
void vbe_draw_rect(int x, int y, int w, int h, unsigned int color);

void outl(unsigned short port, unsigned int val) {
    asm volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

unsigned int inl(unsigned short port) {
    unsigned int ret;
    asm volatile ( "inl %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// Ekran Fonksiyonları
void update_cursor() {
    unsigned short pos = terminal_row * 80 + terminal_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char) ((pos >> 8) & 0xFF));
}

void clear_scr() {
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        video_memory[i] = ' ';
        video_memory[i+1] = 0x07;
    }
    terminal_row = 0;
    terminal_col = 0;
    update_cursor();
}

void scroll() {
    if (terminal_row >= 25) {
        for (int i = 0; i < 24 * 80 * 2; i++) {
            video_memory[i] = video_memory[i + 80 * 2];
        }
        for (int i = 24 * 80 * 2; i < 25 * 80 * 2; i += 2) {
            video_memory[i] = ' ';
            video_memory[i+1] = 0x07;
        }
        terminal_row = 24;
    }
}

void put_char(char c) {
    if (c == '\n') {
        terminal_col = 0;
        terminal_row++;
    } else if (c == '\b') {
        if (terminal_col > 0) {
            terminal_col--;
            int pos = (terminal_row * 80 + terminal_col) * 2;
            video_memory[pos] = ' ';
            video_memory[pos+1] = 0x07;
        }
    } else {
        int pos = (terminal_row * 80 + terminal_col) * 2;
        video_memory[pos] = c;
        video_memory[pos+1] = 0x07;
        terminal_col++;
        if (terminal_col >= 80) {
            terminal_col = 0;
            terminal_row++;
        }
    }
    scroll();
    update_cursor();
}

void put_str(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        put_char(str[i]);
    }
}

void put_int(int n) {
    if (n == 0) {
        put_char('0');
        return;
    }
    if (n < 0) {
        put_char('-');
        n = -n;
    }
    char buf[12];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) {
        put_char(buf[i]);
    }
}

void put_hex(unsigned int n) {
    char *chars = "0123456789ABCDEF";
    put_str("0x");
    for (int i = 7; i >= 0; i--) {
        put_char(chars[(n >> (i * 4)) & 0xF]);
    }
}

// String Fonksiyonları
int strlen(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Test Görevleri
extern unsigned int stack_top;

void task1_func() {
    while(1) {
        ((char*)0xC00B8000)[158] = '1'; 
        ((char*)0xC00B8000)[159] = 0x0E; 
        for(int i=0; i<1000000; i++) asm volatile("nop");
    }
}

void task2_func() {
    while(1) {
        ((char*)0xC00B8000)[156] = '2';
        ((char*)0xC00B8000)[157] = 0x0B; 
        for(int i=0; i<1000000; i++) asm volatile("nop");
    }
}

// Grafik Cizim Gorevi (Task)
void graphics_task() {
    while(1) {
        // SADECE imleci guncelle (Piksel kurtarma yontemi ile cok hizli)
        mouse_state_t* ms = mouse_get_state();
        vbe_draw_cursor(ms->x, ms->y);
        
        // Cok kisa bir bekleme (Islemciyi yormamak icin)
        for(int i = 0; i < 10000; i++) asm volatile("nop");
    }
}

void user_mode_test() {
    while(1) {
        // Sistem çağrısı testi: syscall1(num, param1)
        syscall1(SYS_WRITE, (int)"[User] Ring 3 Syscall Merhaba!\n");
        
        for(int i=0; i<10000000; i++) asm volatile("nop");
    }
}

// Kernel Giriş Noktası
void kernel_main(unsigned int magic, struct multiboot_info* mbi) {
    // 1. GDT ve IDT'yi kur
    init_gdt();
    init_idt();

    // 2. PIC'i yeniden haritala (tüm IRQ'lar kapalı başlar)
    irq_remap();

    // 3. Ekranı temizle ve karşılama mesajı göster
    clear_scr();
    put_str("========================================\n");
    put_str("         PekerOS v0.2 - Kernel\n");
    put_str("========================================\n\n");

    if (magic != MULTIBOOT_MAGIC) {
        put_str("[HATA] Gecersiz Multiboot Magic Number!\n");
        return;
    }

    put_str("[OK] GDT yuklendi.\n");
    put_str("[OK] IDT yuklendi.\n");
    put_str("[OK] PIC yeniden haritalandi.\n");

    // 4. Fiziksel Bellek Yöneticisini (PMM) başlat
    init_pmm(mbi);
    put_str("[OK] Fiziksel Bellek Yoneticisi (PMM) baslatildi.\n");

    // 4.1. Sanal Bellek (Paging) başlat
    init_paging();

    // 4.2. Dinamik Bellek (Kernel Heap) başlat
    init_kheap();

    // Test: kmalloc ve kfree
    int *test_ptr1 = (int *)kmalloc(sizeof(int) * 10);
    int *test_ptr2 = (int *)kmalloc(200);
    put_str("[TEST] ptr1 tahsis edildi: 0x");
    put_hex((unsigned int)test_ptr1);
    put_str("\n[TEST] ptr2 tahsis edildi: 0x");
    put_hex((unsigned int)test_ptr2);
    put_str("\n[TEST] Bellekler serbest birakiliyor...\n");
    kfree(test_ptr1);
    kfree(test_ptr2);

    pafs_init();
    pafs_write("merhaba.txt", "PekerOS Dosya Sistemine Hosgeldiniz!", 37);

    // Donanim Kesfi
    pci_init();

    // Grafik Modu Baslatma
    vbe_init(mbi);
    vbe_draw_gradient(); // Arka plani bir kez ciz
    vbe_draw_rect(100, 100, 200, 150, 0x00E67E22); // Statik kutular
    vbe_draw_rect(400, 300, 100, 100, 0x002ECC71);
    vbe_update(); // Ekrana yansit

    vbe_write("\n   PEKER OS - Graphics Mode Activated\n", 0x00FFFFFF);
    vbe_write("   ----------------------------------\n", 0x00F1C40F);
    vbe_write("   Welcome to the future of PekerOS!\n", 0x00ECf0F1);

    vbe_update(); // HER SEYI EKRANA YANSIT

    // 4.4. Multitasking başlat
    init_multitasking();
    
    // TSS'yi ayarla (GDT index 5, Kernel Stack kullan)
    write_tss(5, 0x10, (unsigned int)&stack_top); 

    create_task("gorev1", task1_func, 0); // Kernel task
    create_task("gorev2", task2_func, 0); // Kernel task
    // create_task("user_task", user_mode_test, 1); // RING 3 TASK! (Su anlik kapali)

    // 5. Zamanlayıcıyı başlat (100 Hz = her 10ms'de bir tick)
    init_timer(100);
    put_str("[OK] Zamanlayici yuklendi (100 Hz).\n");

    // 6. Fare sürücüsünü başlat (IRQ12)
    mouse_init();

    // 5. Klavye sürücüsünü başlat (IRQ1 handler'ı kurar ve IRQ1'i açar)
    init_keyboard();
    put_str("[OK] Klavye surucusu yuklendi.\n");

    // 7. Shell'i başlat
    put_str("[OK] fash shell baslatildi.\n\n");
    init_syscalls();

    // Grafik gorevini baslat
    create_task("graphics", graphics_task, 0);

    init_shell();
    // 7. Kesmeleri etkinleştir ve bekleme döngüsüne gir
    asm volatile("sti");

    for (;;) {
        asm volatile("hlt");
    }
}