#include "gui.h"
#include "common.h"
#include "kheap.h"

static void test_btn_click(gui_widget_t* btn);

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
        // 1. Ikonlara mi tiklandi? (Sol taraf 0-100 arasi)
        if (x < 100) {
            if (y >= 40 && y <= 90) {
                window_t *w = gui_create_window("UI Test Paneli", 200, 150, 400, 250, 0x00F39C12);
                gui_create_label(w, 20, 40, "Merhaba PekerOS GUI!", 0x002F3542);
                gui_create_button(w, 20, 80, 120, 30, "Tikla Bana", test_btn_click);
            }
            else if (y >= 110 && y <= 160) gui_create_window("Terminal", 100, 100, 500, 350, 0x00000000);
            else if (y >= 180 && y <= 230) gui_create_window("Dosyalar", 300, 200, 450, 300, 0x003498DB);
        }

        // 2. Pencerelere mi tiklandi? (En öndekinden ara)
        window_t *curr = window_list_tail;
        while (curr) {
            if (x >= curr->x && x <= curr->x + curr->w &&
                y >= curr->y && y <= curr->y + curr->h) {
                
                // Kapatma butonu...
                if (x >= curr->x + curr->w - 20 && x <= curr->x + curr->w - 5 &&
                    y >= curr->y + 5 && y <= curr->y + 20) {
                    gui_destroy_window(curr);
                    break;
                }

                gui_bring_to_front(curr);
                
                // Drag...
                if (y <= curr->y + 25) {
                    dragged_window = curr;
                    drag_off_x = x - curr->x;
                    drag_off_y = y - curr->y;
                } else {
                    // Widget click check
                    gui_widget_t *w = curr->widgets_head;
                    while (w) {
                        int wx = curr->x + w->x;
                        int wy = curr->y + w->y;
                        if (x >= wx && x <= wx + w->w && y >= wy && y <= wy + w->h) {
                            if (w->on_click) w->on_click(w);
                            break;
                        }
                        w = w->next;
                    }
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

static void test_btn_click(gui_widget_t* btn) {
    if (strcmp(btn->text, "Tikla Bana") == 0) {
        strcpy(btn->text, "Tiklandi!");
        btn->bg_color = 0x002ED573; // Green
    } else {
        strcpy(btn->text, "Tikla Bana");
        btn->bg_color = 0x00DFE4EA; // Default
    }
}

window_t* gui_create_window(char* title, int x, int y, int w, int h, unsigned int color) {
    window_t *win = kmalloc(sizeof(window_t));
    memset(win, 0, sizeof(window_t));
    
    win->x = x; win->y = y; win->w = w; win->h = h;
    win->color = color;
    win->active = 1;
    strcpy(win->title, title);
    
    win->widgets_head = 0;
    win->widgets_tail = 0;

    // Listeye ekle (En öne)
    if (!window_list_head) {
        window_list_head = win;
        window_list_tail = win;
    } else {
        window_list_tail->next = win;
        win->prev = window_list_tail;
        window_list_tail = win;
    }

    return win; 
}

gui_widget_t* gui_create_button(window_t* win, int x, int y, int w, int h, char* text, void (*on_click)(gui_widget_t*)) {
    if (!win) return 0;
    gui_widget_t *btn = kmalloc(sizeof(gui_widget_t));
    memset(btn, 0, sizeof(gui_widget_t));
    
    btn->type = WIDGET_BUTTON;
    btn->x = x; btn->y = y; btn->w = w; btn->h = h;
    strcpy(btn->text, text);
    btn->bg_color = 0x00DFE4EA; 
    btn->fg_color = 0x002F3542; 
    btn->on_click = on_click;
    
    if (!win->widgets_head) {
        win->widgets_head = btn;
        win->widgets_tail = btn;
    } else {
        win->widgets_tail->next = btn;
        btn->prev = win->widgets_tail;
        win->widgets_tail = btn;
    }
    return btn;
}

gui_widget_t* gui_create_label(window_t* win, int x, int y, char* text, unsigned int color) {
    if (!win) return 0;
    gui_widget_t *lbl = kmalloc(sizeof(gui_widget_t));
    memset(lbl, 0, sizeof(gui_widget_t));
    
    lbl->type = WIDGET_LABEL;
    lbl->x = x; lbl->y = y; lbl->w = 0; lbl->h = 0;
    strcpy(lbl->text, text);
    lbl->fg_color = color;
    
    if (!win->widgets_head) {
        win->widgets_head = lbl;
        win->widgets_tail = lbl;
    } else {
        win->widgets_tail->next = lbl;
        lbl->prev = win->widgets_tail;
        win->widgets_tail = lbl;
    }
    return lbl;
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
    
    // 6. Widget'ları Çiz
    gui_widget_t *w = win->widgets_head;
    while (w) {
        int abs_x = win->x + w->x;
        int abs_y = win->y + w->y;
        
        if (w->type == WIDGET_BUTTON) {
            vbe_draw_rect(abs_x, abs_y, w->w, w->h, w->bg_color);
            vbe_draw_rect(abs_x, abs_y, w->w, 1, 0x002F3542);
            vbe_draw_rect(abs_x, abs_y + w->h - 1, w->w, 1, 0x002F3542);
            vbe_draw_rect(abs_x, abs_y, 1, w->h, 0x002F3542);
            vbe_draw_rect(abs_x + w->w - 1, abs_y, 1, w->h, 0x002F3542);
            int text_len = strlen(w->text);
            int text_x = abs_x + (w->w / 2) - (text_len * 4);
            int text_y = abs_y + (w->h / 2) - 8;
            vbe_write_at(text_x, text_y, w->text, w->fg_color);
        } else if (w->type == WIDGET_LABEL) {
            vbe_write_at(abs_x, abs_y, w->text, w->fg_color);
        }
        
        w = w->next;
    }
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
    
    // Panel İçindeki Saat (Gercek Zamanli)
    int h, m, s;
    extern void rtc_get_time(int *h, int *m, int *s);
    rtc_get_time(&h, &m, &s);
    
    char time_str[16];
    // Basit itoa/format mantigi (kprintf olmadigi icin manuel)
    time_str[0] = (h / 10) + '0'; time_str[1] = (h % 10) + '0'; time_str[2] = ':';
    time_str[3] = (m / 10) + '0'; time_str[4] = (m % 10) + '0'; time_str[5] = ':';
    time_str[6] = (s / 10) + '0'; time_str[7] = (s % 10) + '0'; time_str[8] = '\0';
    
    vbe_write_at(360, 8, time_str, 0x002F3640);
}
