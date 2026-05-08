#include "common.h"
#include "idt.h"
#include "task.h"

// VGA Metin belleği adresi (Higher Half)
static char* const vga_mem = (char*) 0xC00B8000;
static int panic_row = 0;
static int panic_col = 0;

static void panic_putc(char c, unsigned char color) {
    if (c == '\n') {
        panic_col = 0;
        panic_row++;
    } else {
        int pos = (panic_row * 80 + panic_col) * 2;
        vga_mem[pos] = c;
        vga_mem[pos+1] = color;
        panic_col++;
        if (panic_col >= 80) {
            panic_col = 0;
            panic_row++;
        }
    }
}

static void panic_puts(const char* s, unsigned char color) {
    while (*s) {
        panic_putc(*s++, color);
    }
}

static void panic_puthex(unsigned int n, unsigned char color) {
    char *chars = "0123456789ABCDEF";
    panic_puts("0x", color);
    for (int i = 7; i >= 0; i--) {
        panic_putc(chars[(n >> (i * 4)) & 0xF], color);
    }
}

void panic(const char *message, struct registers *regs) {
    // 1. Kesmeleri kapat
    asm volatile("cli");

    // 2. Ekranı temizle (Mavi arka plan, Beyaz yazı: 0x1F)
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga_mem[i] = ' ';
        vga_mem[i+1] = 0x1F;
    }

    panic_row = 2;
    panic_col = 25;
    panic_puts("##############################\n", 0x1F);
    panic_col = 25;
    panic_puts("#       !!! KERNEL PANIC !!!     #\n", 0x1F);
    panic_col = 25;
    panic_puts("##############################\n\n", 0x1F);

    panic_col = 5;
    panic_puts("HATA: ", 0x1E); // Sarı yazı
    panic_puts(message, 0x1F);
    panic_puts("\n\n", 0x1F);

    if (regs) {
        panic_col = 5;
        panic_puts("--- REGISTER DOKUMU ---\n", 0x1B); // Açık mavi/cyan
        
        panic_col = 5;
        panic_puts("EAX: ", 0x1F); panic_puthex(regs->eax, 0x1F);
        panic_puts("  EBX: ", 0x1F); panic_puthex(regs->ebx, 0x1F);
        panic_puts("  ECX: ", 0x1F); panic_puthex(regs->ecx, 0x1F);
        panic_puts("\n", 0x1F);

        panic_col = 5;
        panic_puts("EDX: ", 0x1F); panic_puthex(regs->edx, 0x1F);
        panic_puts("  ESI: ", 0x1F); panic_puthex(regs->esi, 0x1F);
        panic_puts("  EDI: ", 0x1F); panic_puthex(regs->edi, 0x1F);
        panic_puts("\n", 0x1F);

        panic_col = 5;
        panic_puts("EBP: ", 0x1F); panic_puthex(regs->ebp, 0x1F);
        panic_puts("  ESP: ", 0x1F); panic_puthex(regs->esp, 0x1F);
        panic_puts("  EIP: ", 0x1F); panic_puthex(regs->eip, 0x1F);
        panic_puts("\n", 0x1F);

        panic_col = 5;
        panic_puts("CS:  ", 0x1F); panic_puthex(regs->cs, 0x1F);
        panic_puts("  DS:  ", 0x1F); panic_puthex(regs->ds, 0x1F);
        panic_puts("  EFLAGS: ", 0x1F); panic_puthex(regs->eflags, 0x1F);
        panic_puts("\n\n", 0x1F);
    }

    task_t* current = get_current_task();
    if (current) {
        panic_col = 5;
        panic_puts("AKTIF GOREV: ", 0x1A); // Yeşilimsi
        panic_puts(current->name, 0x1F);
        panic_puts("\n", 0x1F);
    }

    panic_row = 22;
    panic_col = 5;
    panic_puts("Sistem durduruldu. Lutfen bilgisayarinizi yeniden baslatin.", 0x1F);

    // 4. Sonsuz döngü
    for (;;) {
        asm volatile("hlt");
    }
}
