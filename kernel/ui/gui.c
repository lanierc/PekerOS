#include "gui.h"
#include "common.h"
#include "kheap.h"

static window_t *window_list_head = 0;
static window_t *window_list_tail = 0;

void gui_init() {
    window_list_head = 0;
    window_list_tail = 0;
}

void gui_destroy_window(window_t *win) {
    if (!win) return;

    // Listeden cikar
    if (win->prev) win->prev->next = win->next;
    if (win->next) win->next->prev = win->prev;
    if (win == window_list_head) window_list_head = win->next;
    if (win == window_list_tail) window_list_tail = win->prev;

    kfree(win);
}

void gui_bring_to_front(window_t *win) {
    if (!win || win == window_list_tail) return;

    // Listeden cikar
    if (win->prev) win->prev->next = win->next;
    if (win->next) win->next->prev = win->prev;
    if (win == window_list_head) window_list_head = win->next;

    // En sona ekle
    win->next = 0;
    win->prev = window_list_tail;
    if (window_list_tail) window_list_tail->next = win;
    window_list_tail = win;
    if (!window_list_head) window_list_head = win;
}

void gui_on_mouse_event(int x, int y, int buttons) {
    static int last_buttons = 0;
    static window_t *dragged_window = 0;
    static int drag_off_x = 0, drag_off_y = 0;

    int left_click = (buttons & 1) && !(last_buttons & 1);
    int left_release = !(buttons & 1) && (last_buttons & 1);

    if (left_click) {
        // En öndeki pencereden başlayarak (arkadan öne değil!) ara
        window_t *curr = window_list_tail;
        while (curr) {
            if (x >= curr->x && x <= curr->x + curr->w &&
                y >= curr->y && y <= curr->y + curr->h) {
                
                // Kapatma butonuna mı tıklandı? (X butonu: win->w - 20)
                if (x >= curr->x + curr->w - 20 && x <= curr->x + curr->w - 5 &&
                    y >= curr->y + 5 && y <= curr->y + 20) {
                    gui_destroy_window(curr);
                    break;
                }

                gui_bring_to_front(curr);
                
                // Başlık çubuğuna mı tıklandı? (Drag kontrolü)
                if (y <= curr->y + 25) {
                    dragged_window = curr;
                    drag_off_x = x - curr->x;
                    drag_off_y = y - curr->y;
                }
                break;
            }
            curr = curr->prev;
        }
    }

    if (left_release) {
        dragged_window = 0;
    }

    if (dragged_window && (buttons & 1)) {
        dragged_window->x = x - drag_off_x;
        dragged_window->y = y - drag_off_y;
    }

    last_buttons = buttons;
}

int gui_create_window(char* title, int x, int y, int w, int h, unsigned int color) {
    window_t *win = kmalloc(sizeof(window_t));
    memset(win, 0, sizeof(window_t));
    
    win->x = x; win->y = y; win->w = w; win->h = h;
    win->color = color;
    win->active = 1;
    strcpy(win->title, title);

    // Listeye ekle (En öne)
    if (!window_list_head) {
        window_list_head = win;
        window_list_tail = win;
    } else {
        window_list_tail->next = win;
        win->prev = window_list_tail;
        window_list_tail = win;
    }

    return 0; // Şimdilik handle sistemi yerine node pointer kullanılacak
}

void gui_draw_icon(int x, int y, char* label, unsigned int color) {
    // Ikon Govdesi (Sembolik kare)
    vbe_draw_rect(x + 5, y, 32, 32, color);
    vbe_draw_rect(x + 5, y, 32, 1, 0x00FFFFFF); // Ust parilti
    
    // Ikon Etiketi
    vbe_write_at(x, y + 35, label, 0x00FFFFFF);
}

void gui_draw_window(window_t* win) {
    if (!win->active) return;

    // 0. Gölge Efekti (Daha yumusak)
    vbe_draw_rect(win->x + 3, win->y + 3, win->w, win->h, 0x002F3640); 

    // 1. Pencere Gövdesi (SkyOS White/Light Gray)
    vbe_draw_rect(win->x, win->y, win->w, win->h, 0x00F1F2F6);
    
    // 2. Başlık Çubuğu (SkyOS Light Blue)
    vbe_draw_rect(win->x, win->y, win->w, 25, 0x0074B9FF); 
    
    // 3. Kapatma Butonu (Sağ üstte SkyOS tarzi)
    vbe_draw_rect(win->x + win->w - 20, win->y + 5, 15, 15, 0x00FF4757);
    
    // 4. Kenarlıklar (Soft Blue/Gray)
    vbe_draw_rect(win->x, win->y, win->w, 1, 0x002F3542);
    vbe_draw_rect(win->x, win->y + win->h - 1, win->w, 1, 0x002F3542);
    vbe_draw_rect(win->x, win->y, 1, win->h, 0x002F3542);
    vbe_draw_rect(win->x + win->w - 1, win->y, 1, win->h, 0x002F3542);

    // 5. Başlık Yazısı (Siyah/Koyu Lacivert)
    vbe_write_at(win->x + 10, win->y + 5, win->title, 0x002F3542);
}

void gui_render() {
    // 1. SkyOS Tarzi Arka Plan (Mavi Gradyan)
    for (int y = 0; y < 600; y++) {
        unsigned char b = 180 + (y * 70) / 600;
        unsigned char g = 120 + (y * 40) / 600;
        vbe_draw_rect(0, y, 800, 1, (g << 8) | b);
    }
    
    // 2. Masaüstü Simgeleri (Sol Taraf)
    gui_draw_icon(20, 40, "Storage", 0x00ffa502);
    gui_draw_icon(20, 110, "Terminal", 0x002f3542);
    gui_draw_icon(20, 180, "Files", 0x001e90ff);
    gui_draw_icon(20, 250, "Network", 0x002ed573);
    
    // 3. Pencereleri Çiz (Arkadan Öne)
    window_t *curr = window_list_head;
    while (curr) {
        if (curr->active) {
            gui_draw_window(curr);
        }
        curr = curr->next;
    }
    
    // 4. SkyOS Üst Panel (Top Bar) - Ortalanmış ve şık
    vbe_draw_rect(300, 0, 200, 30, 0x00DCDDE1); // Panel gövdesi
    vbe_draw_rect(300, 30, 200, 1, 0x007F8C8D);  // Alt çizgi
    
    // Panel İçindeki Saat (Temsili)
    vbe_write_at(360, 8, "12:45 PM", 0x002F3640);
}
