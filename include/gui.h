#ifndef GUI_H
#define GUI_H

#include "vbe.h"

#define MAX_WINDOWS 10

typedef struct window {
    int x, y;
    int w, h;
    char title[64];
    unsigned int color;
    int active;
    int focus;
    int is_dragging;
    
    struct window *next;
    struct window *prev;
} window_t;

void gui_init();
void gui_render();
void gui_on_mouse_event(int x, int y, int buttons);
int gui_create_window(char* title, int x, int y, int w, int h, unsigned int color);

#endif
