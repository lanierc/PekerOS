#include "gui.h"
#include "common.h"
#include "kheap.h"

static window_t windows[MAX_WINDOWS];
static int window_count = 0;

void gui_init() {
    window_count = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].active = 0;
    }
}

int gui_create_window(char* title, int x, int y, int w, int h, unsigned int color) {
    if (window_count >= MAX_WINDOWS) return -1;
    
    windows[window_count].x = x;
    windows[window_count].y = y;
    windows[window_count].w = w;
    windows[window_count].h = h;
    windows[window_count].color = color;
    windows[window_count].active = 1;
    
    int i = 0;
    while (title[i] && i < 63) {
        windows[window_count].title[i] = title[i];
        i++;
    }
    windows[window_count].title[i] = '\0';
    
    return window_count++;
}

void gui_draw_window(window_t* win) {
    if (!win->active) return;

    // 1. Pencere Gövdesi (Window Body)
    vbe_draw_rect(win->x, win->y, win->w, win->h, win->color);
    
    // 2. Başlık Çubuğu (Title Bar)
    vbe_draw_rect(win->x, win->y, win->w, 25, 0x002C3E50); // Koyu lacivert başlık çubuğu
    
    // 3. Kenarlık (Border)
    vbe_draw_rect(win->x, win->y, win->w, 1, 0x00FFFFFF);
    vbe_draw_rect(win->x, win->y + win->h - 1, win->w, 1, 0x00FFFFFF);
    vbe_draw_rect(win->x, win->y, 1, win->h, 0x00FFFFFF);
    vbe_draw_rect(win->x + win->w - 1, win->y, 1, win->h, 0x00FFFFFF);
    
    // 4. Başlık Yazısı
    // Not: vbe_write cursor kullanıyor, bu yüzden pencere içine yazı yazmak için özel koordinat lazım.
    // Şimdilik sadece pencereyi çiziyoruz.
}

void gui_render() {
    // 1. Arka Planı Çiz (Desktop Wallpaper)
    vbe_draw_gradient(); 
    
    // 2. Pencereleri Çiz
    for (int i = 0; i < window_count; i++) {
        if (windows[i].active) {
            gui_draw_window(&windows[i]);
        }
    }
    
    // 3. Görev Çubuğu (Taskbar)
    vbe_draw_rect(0, 560, 800, 40, 0x002C3E50);
    vbe_draw_rect(5, 565, 80, 30, 0x00E74C3C); // "Başlat" butonu temsili
}
