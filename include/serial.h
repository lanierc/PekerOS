#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"

#define COM1 0x3F8

void init_serial();
void serial_write(char c);
void serial_print(const char* str);

#endif
