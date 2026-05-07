#ifndef GUI_H
#define GUI_H

#include "vbe.h"

#define MAX_WINDOWS 10

typedef struct {
    int x, y;
    int w, h;
    char title[64];
    unsigned int color;
    int active;
} window_t;

void gui_init();
void gui_render();
int gui_create_window(char* title, int x, int y, int w, int h, unsigned int color);

#endif
