#include "idt.h"
#include "shell.h"

// Türkçe Q Klavye Düzeni (Scancode Set 1)
static int shift_pressed = 0;

// Boyutu derleyiciye bırakalım [] kullanarak
static const unsigned char scancode_normal[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '*', '-', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 'g', 0x81, '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 's', 'i', '"', 0,
    ',', 'z', 'x', 'c', 'v', 'b', 'n', 'm', 0x94, 0x87, '.', 0, '*', 0, ' ', 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const unsigned char scancode_shift[] = {
    0, 27, '!', '\'', '^', '+', '%', '&', '/', '(', ')', '=', '?', '_', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 'G', 0x9A, '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'S', 'I', '\'', 0,
    ';', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', 0x99, 0x80, ':', 0, '*', 0, ' ', 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void keyboard_handler(struct registers *r) {
    unsigned char scancode = inb(0x60);

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }
    if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) {
        shift_pressed = 0;
        return;
    }

    if (scancode & 0x80) return;

    // Yön tuşları
    if (scancode == 0x48) { // Yukarı ok
        shell_input('\x11');
        return;
    }
    if (scancode == 0x50) { // Aşağı ok
        shell_input('\x12');
        return;
    }

    // Scancode dizinin boyutundan büyükse işlem yapma (güvenlik için)
    if (scancode >= sizeof(scancode_normal)) return;

    unsigned char c = (shift_pressed) ? scancode_shift[scancode] : scancode_normal[scancode];

    if (c) {
        shell_input(c);
    }
}

void init_keyboard() {
    // Klavye handler'ını kaydet
    irq_install_handler(1, keyboard_handler);

    // IRQ1'i (klavye) aç - Master PIC maskeleme kaydını güncelle
    unsigned char mask = inb(0x21);
    mask &= ~(1 << 1);  // Bit 1'i temizle = IRQ1 açık
    outb(0x21, mask);
}