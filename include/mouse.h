#ifndef MOUSE_H
#define MOUSE_H

#include "common.h"

// Mouse Durumları
typedef struct {
    unsigned char x_relative;
    unsigned char y_relative;
    unsigned char buttons;
    int x;
    int y;
} mouse_state_t;

// Fonksiyonlar
void mouse_init();
void mouse_handler(struct registers *r);
mouse_state_t* mouse_get_state();

#endif
