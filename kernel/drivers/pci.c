#include "pci.h"

// PCI Konfigürasyon alanından 16-bit veri oku
unsigned short pci_config_read_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address;
    unsigned int lbus  = (unsigned int)bus;
    unsigned int lslot = (unsigned int)slot;
    unsigned int lfunc = (unsigned int)func;
    unsigned short tmp = 0;

    // Adres oluştur: bit 31 (enable), bits 23-16 (bus), bits 15-11 (device), bits 10-8 (function), bits 7-0 (offset)
    address = (unsigned int)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));

    outl(PCI_CONFIG_ADDRESS, address);
    
    // Veriyi oku ve gerekli 16 biti döndür
    tmp = (unsigned short)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

unsigned int pci_config_read_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned short value) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    unsigned int old = inl(PCI_CONFIG_DATA);
    old = (old & ~(0xFFFF << ((offset & 2) * 8))) | (value << ((offset & 2) * 8));
    outl(PCI_CONFIG_DATA, old);
}

void pci_config_write_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int value) {
    unsigned int address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

// PCI Class kodlarını metne dönüştür
const char* pci_class_to_str(unsigned char class_code) {
    switch (class_code) {
        case 0x00: return "Unclassified";
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge Device";
        case 0x07: return "Simple Communication Controller";
        case 0x08: return "Base System Peripheral";
        case 0x09: return "Input Device Controller";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        case 0x0E: return "Intelligent Controller";
        case 0x0F: return "Satellite Communication Controller";
        case 0x10: return "Encryption Controller";
        case 0x11: return "Signal Processing Controller";
        case 0xFF: return "Unassigned Class";
        default: return "Unknown";
    }
}

// PCI Subclass kodlarını metne dönüştür (En önemli olanlar)
const char* pci_subclass_to_str(unsigned char class_code, unsigned char subclass_code) {
    if (class_code == 0x01) { // Mass Storage
        switch (subclass_code) {
            case 0x01: return "IDE Interface";
            case 0x04: return "RAID Bus Controller";
            case 0x05: return "ATA Controller";
            case 0x06: return "SATA Controller";
            case 0x08: return "Non-Volatile Memory Controller (NVMe)";
            default: return "Storage Device";
        }
    } else if (class_code == 0x02) { // Network
        switch (subclass_code) {
            case 0x00: return "Ethernet Controller";
            case 0x01: return "Token Ring Controller";
            default: return "Network Interface";
        }
    } 
    else if (class_code == 0x03) { // Display
        switch (subclass_code) {
            case 0x00: return "VGA Compatible Controller";
            default: return "Video Controller";
        }
    } else if (class_code == 0x0C) { // Serial Bus
        switch (subclass_code) {
            case 0x03: return "USB Controller";
            case 0x05: return "SMBus";
            default: return "Bus Controller";
        }
    }
    return "Device";
}

// PCI Aygıt Tarama ve Listeleme
void pci_init() {
    put_str("[PCI] Donanim taraniyor...\n");
    put_str("BUS  DEV  FUNC  VENDOR  DEVICE  CLASS  DESCRIPTION\n");
    put_str("---  ---  ----  ------  ------  -----  -----------\n");

    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            for (int func = 0; func < 8; func++) {
                unsigned short vendor = pci_config_read_word(bus, dev, func, 0);
                if (vendor == 0xFFFF) continue; // Cihaz yok

                unsigned short device = pci_config_read_word(bus, dev, func, 2);
                unsigned short class_sub = pci_config_read_word(bus, dev, func, 10);
                unsigned char class_code = (class_sub >> 8) & 0xFF;
                unsigned char subclass_code = class_sub & 0xFF;

                // Ekrana yazdır (Detaylı)
                put_int(bus); put_str("    ");
                put_int(dev); put_str("    ");
                put_int(func); put_str("     ");
                put_hex(vendor); put_str("  ");
                put_hex(device); put_str("  ");
                put_hex(class_code); put_str("    ");
                put_str(pci_class_to_str(class_code));
                put_str(" (");
                put_str(pci_subclass_to_str(class_code, subclass_code));
                put_str(")\n");

                // RTL8139 kontrolü (Vendor: 0x10EC, Device: 0x8139)
                if (vendor == 0x10EC && device == 0x8139) {
                    put_str("\n[RTL8139] Cihaz PCI uzerinde bulundu!\n");
                    
                    // Bus Mastering (Bit 2 of Command Register at offset 0x04)
                    unsigned short command = pci_config_read_word(bus, dev, func, 0x04);
                    command |= (1 << 2);
                    pci_config_write_word(bus, dev, func, 0x04, command);
                    
                    // I/O Port Base (BAR0, offset 0x10)
                    unsigned int bar0 = pci_config_read_dword(bus, dev, func, 0x10);
                    unsigned int io_base = bar0 & ~3; // I/O adresi için son iki biti temizle
                    
                    // IRQ Line (offset 0x3C, lowest byte)
                    unsigned int irq = pci_config_read_dword(bus, dev, func, 0x3C) & 0xFF;
                    
                    // Sürücüyü başlat
                    extern void init_rtl8139(unsigned int io_base, unsigned char irq);
                    init_rtl8139(io_base, irq);
                }
                else if (vendor == 0x8086 && (device == 0x100E || device == 0x100F)) {
                    put_str("\n[Intel E1000] Cihaz PCI uzerinde bulundu!\n");
                    
                    // Bus Mastering
                    unsigned short command = pci_config_read_word(bus, dev, func, 0x04);
                    command |= (1 << 2);
                    pci_config_write_word(bus, dev, func, 0x04, command);
                    
                    // MMIO Base (BAR0, offset 0x10)
                    unsigned int bar0 = pci_config_read_dword(bus, dev, func, 0x10);
                    unsigned int phys_addr = bar0 & ~0xF; 
                    
                    // IRQ Line
                    unsigned int irq = pci_config_read_dword(bus, dev, func, 0x3C) & 0xFF;
                    
                    // Sürücüyü başlat
                    extern void init_e1000(unsigned int phys_addr, unsigned char irq);
                    init_e1000(phys_addr, irq);
                }
                // Eğer cihaz tek fonksiyonluysa diğer fonksiyonları taramaya gerek yok
                if (func == 0) {
                    unsigned short header_type = pci_config_read_word(bus, dev, 0, 0x0E);
                    if (!(header_type & 0x80)) break;
                }
            }
        }
    }
}
