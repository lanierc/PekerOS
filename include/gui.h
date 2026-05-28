#ifndef GUI_H
#define GUI_H

#include "vbe.h"

#define MAX_WINDOWS 10

enum gui_event_type {
    GUI_EVENT_CLICK,
    GUI_EVENT_MOUSE_ENTER,
    GUI_EVENT_MOUSE_LEAVE,
    GUI_EVENT_KEYPRESS
};

enum gui_widget_type {
    WIDGET_BUTTON,
    WIDGET_LABEL,
    WIDGET_TEXTBOX
};

typedef struct gui_widget {
    int type;
    int x, y;
    int w, h;
    char text[64];
    unsigned int bg_color;
    unsigned int fg_color;
    
    void (*on_click)(struct gui_widget* sender);
    
    struct gui_widget *next;
    struct gui_widget *prev;
} gui_widget_t;

typedef struct window {
    int x, y;
    int w, h;
    char title[64];
    unsigned int color;
    int active;
    int focus;
    int is_dragging;
    
    gui_widget_t *widgets_head;
    gui_widget_t *widgets_tail;
    
    struct window *next;
    struct window *prev;
} window_t;

void gui_init();
void gui_render();
void gui_on_mouse_event(int x, int y, int buttons);
window_t* gui_create_window(char* title, int x, int y, int w, int h, unsigned int color);
gui_widget_t* gui_create_button(window_t* win, int x, int y, int w, int h, char* text, void (*on_click)(gui_widget_t*));
gui_widget_t* gui_create_label(window_t* win, int x, int y, char* text, unsigned int color);

#endif
