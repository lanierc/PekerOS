#ifndef VBE_H
#define VBE_H

#include "multiboot.h"

// Grafik Modu Bilgileri
typedef struct {
    unsigned int *address;
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned char bpp;
} vbe_info_t;

// Fonksiyonlar
void vbe_init(struct multiboot_info *mb_info);
void vbe_put_pixel(int x, int y, unsigned int color);
void vbe_clear_screen(unsigned int color);
void vbe_draw_rect(int x, int y, int w, int h, unsigned int color);
void vbe_draw_gradient();
void vbe_draw_char(int x, int y, char c, unsigned int color);
void vbe_write(const char *str, unsigned int color);
void vbe_draw_cursor(int x, int y);
void vbe_update();
void vbe_update_rect(int x, int y, int w, int h);

int vbe_is_active();
void vbe_set_active(int active);

#endif
