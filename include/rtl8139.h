#ifndef RTL8139_H
#define RTL8139_H

#include "common.h"

// RTL8139 Registers
#define RTL8139_MAC05     0x00
#define RTL8139_MAR07     0x08
#define RTL8139_TSD0      0x10
#define RTL8139_TSAD0     0x20
#define RTL8139_RBSTART   0x30
#define RTL8139_CR        0x37
#define RTL8139_CAPR      0x38
#define RTL8139_CBR       0x3A
#define RTL8139_IMR       0x3C
#define RTL8139_ISR       0x3E
#define RTL8139_TCR       0x40
#define RTL8139_RCR       0x44
#define RTL8139_TAD       0x4C
#define RTL8139_CONFIG1   0x52

void init_rtl8139(unsigned int base_addr, unsigned char irq);
void rtl8139_send_packet(void *data, int len);

#endif
