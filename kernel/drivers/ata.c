#include "ata.h"
#include "spinlock.h"

extern void outb(unsigned short port, unsigned char val);
extern unsigned char inb(unsigned short port);
extern void outw(unsigned short port, unsigned short val);
extern unsigned short inw(unsigned short port);
extern void kprintf(const char* format, ...);

static spinlock_t ata_lock = SPINLOCK_INIT;

// BSY (Busy) ve DRQ (Data Request) durumlarını bekle
int ata_wait_ready(void) {
    int timeout = 1000000;
    while (--timeout) {
        unsigned char status = inb(ATA_PRIMARY_STATUS);
        if (status == 0xFF) return -1; // Sürücü yok
        // BSY 0 olmalı ve DRQ hazır olmalı (veya hata olmamalı)
        if (!(status & 0x80)) return 0;
    }
    return -1;
}

int ata_wait_bsy(void) {
    int timeout = 1000000;
    while ((inb(ATA_PRIMARY_STATUS) & 0x80) && --timeout) {
        if (inb(ATA_PRIMARY_STATUS) == 0xFF) return -1;
    }
    return timeout ? 0 : -1;
}

int ata_wait_drq(void) {
    int timeout = 1000000;
    while (!(inb(ATA_PRIMARY_STATUS) & 0x08) && --timeout) {
        unsigned char status = inb(ATA_PRIMARY_STATUS);
        if (status == 0xFF) return -1;
        if (status & 0x01) return -1; // ERR bit set
    }
    return timeout ? 0 : -1;
}

// LBA28 modunda 1 sektör (512 byte) oku
void ata_read_sector(unsigned char drive_idx, unsigned int lba, unsigned char *buffer) {
    spin_lock(&ata_lock);
    
    unsigned short port = (drive_idx < 2) ? 0x1F0 : 0x170;
    unsigned char drive = (drive_idx % 2 == 0) ? 0xE0 : 0xF0;

    outb(port + 6, drive | ((lba >> 24) & 0x0F));
    for(int i=0; i<4; i++) inb(port + 7); 
    
    if (inb(port + 7) == 0xFF) {
        spin_unlock(&ata_lock);
        return;
    }

    int timeout = 1000000;
    while ((inb(port + 7) & 0x80) && --timeout);
    
    if (timeout == 0) {
        unsigned char status = inb(port + 7);
        kprintf("[HATA] ATA Okuma Zamanasimi! Port: %x, Status: %x\n", port, status);
        spin_unlock(&ata_lock);
        return;
    }

    outb(port + 2, 1);
    outb(port + 3, (unsigned char)(lba));
    outb(port + 4, (unsigned char)(lba >> 8));
    outb(port + 5, (unsigned char)(lba >> 16));
    outb(port + 7, 0x20);
    
    // DRQ bekle (Her port için ayrı bekleme lazım ama basitlik için genel bekliyoruz)
    // Aslında ata_wait_drq içinde port parametresi yok, sabit 0x1F7 bakıyor.
    // Düzeltmemiz lazım.
    
    // Port bazlı DRQ bekleme
    timeout = 1000000;
    while (!(inb(port + 7) & 0x08) && --timeout);

    if (timeout == 0) {
        spin_unlock(&ata_lock);
        return;
    }
    
    for (int i = 0; i < 256; i++) {
        unsigned short word = inw(port);
        buffer[i * 2] = (unsigned char)(word & 0xFF);
        buffer[i * 2 + 1] = (unsigned char)((word >> 8) & 0xFF);
    }
    spin_unlock(&ata_lock);
}

// LBA28 modunda 1 sektör (512 byte) yaz
void ata_write_sector(unsigned char drive_idx, unsigned int lba, unsigned char *buffer) {
    spin_lock(&ata_lock);
    
    unsigned short port = (drive_idx < 2) ? 0x1F0 : 0x170;
    unsigned char drive = (drive_idx % 2 == 0) ? 0xE0 : 0xF0;

    outb(port + 6, drive | ((lba >> 24) & 0x0F));
    for(int i=0; i<4; i++) inb(port + 7);

    int timeout = 1000000;
    while ((inb(port + 7) & 0x80) && --timeout);
    if (timeout == 0) {
        unsigned char status = inb(port + 7);
        kprintf("[HATA] ATA Yazma Hatasi (BSY mesgul: %x)! Drive: %d\n", status, drive_idx);
        spin_unlock(&ata_lock);
        return;
    }
    
    outb(port + 2, 1);
    outb(port + 3, (unsigned char)(lba));
    outb(port + 4, (unsigned char)(lba >> 8));
    outb(port + 5, (unsigned char)(lba >> 16));
    outb(port + 7, 0x30);
    
    timeout = 1000000;
    while (!(inb(port + 7) & 0x08) && --timeout);
    if (timeout == 0) {
        spin_unlock(&ata_lock);
        return;
    }
    
    for (int i = 0; i < 256; i++) {
        unsigned short word = (unsigned short)(buffer[i * 2]) | ((unsigned short)(buffer[i * 2 + 1]) << 8);
        outw(port, word);
    }
    
    // Flush
    outb(port + 7, 0xE7);
    for(int i=0; i<4; i++) inb(port + 7);
    timeout = 1000000;
    while ((inb(port + 7) & 0x80) && --timeout);
    
    spin_unlock(&ata_lock);
}
