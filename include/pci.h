#ifndef PCI_H
#define PCI_H

#include "common.h"

// PCI I/O Portları
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// PCI Cihaz Yapısı
typedef struct {
    unsigned char bus;
    unsigned char device;
    unsigned char function;
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned char class_code;
    unsigned char subclass_code;
    unsigned char prog_if;
    unsigned char revision_id;
} pci_dev_t;

// Fonksiyonlar
unsigned short pci_config_read_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset);
unsigned int pci_config_read_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset);
void pci_config_write_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned short value);
void pci_config_write_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int value);
void pci_init();
const char* pci_class_to_str(unsigned char class_code);
const char* pci_subclass_to_str(unsigned char class_code, unsigned char subclass_code);

#endif
