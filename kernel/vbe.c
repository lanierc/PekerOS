#include "vbe.h"
#include "common.h"
#include "paging.h"
#include "pci.h"

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF
#define VBE_DISPI_INDEX_XRES   0x1
#define VBE_DISPI_INDEX_YRES   0x2
#define VBE_DISPI_INDEX_BPP    0x3
#define VBE_DISPI_INDEX_ENABLE 0x4
#define VBE_DISPI_DISABLED     0x00
#define VBE_DISPI_ENABLED      0x01
#define VBE_DISPI_LFB_ENABLED  0x40

extern unsigned char font8x16[256][16];
static vbe_info_t vbe_info;
static unsigned int* back_buffer = (unsigned int*)0xD0000000;
static unsigned int mouse_bg_save[64]; // Fare altindaki orijinal pikselleri saklamak icin

static void bga_write(unsigned short index, unsigned short data) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, data);
}

void vbe_init(struct multiboot_info *mb_info) {
    unsigned int lfb_phys = 0;
    if (mb_info->flags & (1 << 12)) {
        lfb_phys = mb_info->framebuffer_addr;
        vbe_info.width = mb_info->framebuffer_width;
        vbe_info.height = mb_info->framebuffer_height;
        vbe_info.pitch = mb_info->framebuffer_pitch;
        vbe_info.bpp = mb_info->framebuffer_bpp;
    } else {
        lfb_phys = (pci_config_read_word(0, 2, 0, 0x12) << 16) | pci_config_read_word(0, 2, 0, 0x10);
        lfb_phys &= 0xFFFFFFF0;
        if (lfb_phys == 0 || lfb_phys == 0xFFFFFFFF) lfb_phys = 0xFD000000;
        bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
        bga_write(VBE_DISPI_INDEX_XRES, 800);
        bga_write(VBE_DISPI_INDEX_YRES, 600);
        bga_write(VBE_DISPI_INDEX_BPP, 32);
        bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
        vbe_info.width = 800;
        vbe_info.height = 600;
        vbe_info.pitch = 800 * 4;
        vbe_info.bpp = 32;
    }
    vbe_info.address = (unsigned int*)lfb_phys;
    paging_map_memory(lfb_phys, lfb_phys, vbe_info.width * vbe_info.height * 4);
    paging_map_memory(0x02000000, 0xD0000000, vbe_info.width * vbe_info.height * 4);
}

void vbe_put_pixel(int x, int y, unsigned int color) {
    if (x < 0 || x >= vbe_info.width || y < 0 || y >= vbe_info.height) return;
    back_buffer[y * vbe_info.width + x] = color;
}

unsigned int vbe_get_pixel(int x, int y) {
    if (x < 0 || x >= vbe_info.width || y < 0 || y >= vbe_info.height) return 0;
    return back_buffer[y * vbe_info.width + x];
}

void vbe_update() {
    unsigned int *src = back_buffer;
    unsigned int *dest = vbe_info.address;
    int size = (vbe_info.width * vbe_info.height);
    for(int i = 0; i < size; i++) {
        dest[i] = src[i];
    }
}

// Belirli bir alani (rect) guncelle (Daha hizli!)
void vbe_update_rect(int x, int y, int w, int h) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int px = x + j;
            int py = y + i;
            if (px < 0 || px >= vbe_info.width || py < 0 || py >= vbe_info.height) continue;
            vbe_info.address[py * vbe_info.width + px] = back_buffer[py * vbe_info.width + px];
        }
    }
}

void vbe_clear_screen(unsigned int color) {
    int size = (vbe_info.width * vbe_info.height);
    for(int i = 0; i < size; i++) {
        back_buffer[i] = color;
    }
}

void vbe_draw_gradient() {
    for (int y = 0; y < vbe_info.height; y++) {
        unsigned char g = (y * 40) / vbe_info.height;
        unsigned char b = (y * 80) / vbe_info.height;
        unsigned int color = (g << 8) | b;
        for (int x = 0; x < vbe_info.width; x++) {
            vbe_put_pixel(x, y, color);
        }
    }
}

void vbe_draw_rect(int x, int y, int w, int h, unsigned int color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            vbe_put_pixel(x + j, y + i, color);
        }
    }
}

void vbe_draw_char(int x, int y, char c, unsigned int color) {
    for (int i = 0; i < 16; i++) {
        unsigned char row = font8x16[(unsigned char)c][i];
        for (int j = 0; j < 8; j++) {
            if (row & (0x80 >> j)) {
                vbe_put_pixel(x + j, y + i, color);
            }
        }
    }
}

static int cursor_x = 0;
static int cursor_y = 0;

void vbe_write(const char *str, unsigned int color) {
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y += 16;
        } else {
            vbe_draw_char(cursor_x, cursor_y, *str, color);
            cursor_x += 8;
            if (cursor_x >= vbe_info.width) {
                cursor_x = 0;
                cursor_y += 16;
            }
        }
        str++;
    }
}

static int last_cursor_x = -1;
static int last_cursor_y = -1;

void vbe_draw_cursor(int x, int y) {
    unsigned char cursor[8][8] = {
        {2,2,0,0,0,0,0,0},
        {2,1,2,0,0,0,0,0},
        {2,1,1,2,0,0,0,0},
        {2,1,1,1,2,0,0,0},
        {2,1,1,1,1,2,0,0},
        {2,1,1,2,2,2,0,0},
        {2,1,2,0,0,0,0,0},
        {2,2,0,0,0,0,0,0}
    };

    // 1. Eski konumu onar (Arka plani geri yukle)
    if (last_cursor_x != -1) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                unsigned int original_pixel = mouse_bg_save[i * 8 + j];
                vbe_put_pixel(last_cursor_x + j, last_cursor_y + i, original_pixel);
            }
        }
        vbe_update_rect(last_cursor_x, last_cursor_y, 8, 8);
    }

    // 2. Yeni konumdaki arka plani kaydet
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            mouse_bg_save[i * 8 + j] = vbe_get_pixel(x + j, y + i);
        }
    }

    // 3. Fare imlecini ciz
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (cursor[i][j] == 1) vbe_put_pixel(x + j, y + i, 0xFFFFFF);
            else if (cursor[i][j] == 2) vbe_put_pixel(x + j, y + i, 0x000000);
        }
    }

    vbe_update_rect(x, y, 8, 8);
    last_cursor_x = x;
    last_cursor_y = y;
}
