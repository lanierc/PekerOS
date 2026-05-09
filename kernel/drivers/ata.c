#include "ata.h"
#include "spinlock.h"

extern void outb(unsigned short port, unsigned char val);
extern unsigned char inb(unsigned short port);
extern void outw(unsigned short port, unsigned short val);
extern unsigned short inw(unsigned short port);

static spinlock_t ata_lock = SPINLOCK_INIT;

// BSY (Busy) bitinin 0 olmasını bekle
void ata_wait_bsy(void) {
    while (inb(ATA_PRIMARY_STATUS) & 0x80);
}

// DRQ (Data Request) bitinin 1 olmasını bekle
void ata_wait_drq(void) {
    while (!(inb(ATA_PRIMARY_STATUS) & 0x08));
}

// LBA28 modunda 1 sektör (512 byte) oku
void ata_read_sector(unsigned char drive, unsigned int lba, unsigned char *buffer) {
    spin_lock(&ata_lock);
    
    // Once surucuyu sec (Master: 0xE0, Slave: 0xF0)
    outb(ATA_PRIMARY_DRV_HEAD, (drive == 0 ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
    // Surucu degistikten sonra 400ns bekleme
    for(int i=0; i<4; i++) inb(ATA_PRIMARY_STATUS); 
    
    // Sürücü hazır olana kadar bekle (BSY ve DRQ temiz olmalı)
    while (inb(ATA_PRIMARY_STATUS) & 0x88); 
    
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LO, (unsigned char)(lba));
    outb(ATA_PRIMARY_LBA_MID, (unsigned char)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (unsigned char)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, 0x20);
    
    ata_wait_bsy();
    ata_wait_drq();
    
    for (int i = 0; i < 256; i++) {
        unsigned short word = inw(ATA_PRIMARY_DATA);
        buffer[i * 2] = (unsigned char)(word & 0xFF);
        buffer[i * 2 + 1] = (unsigned char)((word >> 8) & 0xFF);
    }
    spin_unlock(&ata_lock);
}

// LBA28 modunda 1 sektör (512 byte) yaz
void ata_write_sector(unsigned char drive, unsigned int lba, unsigned char *buffer) {
    spin_lock(&ata_lock);
    
    outb(ATA_PRIMARY_DRV_HEAD, (drive == 0 ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
    for(int i=0; i<4; i++) inb(ATA_PRIMARY_STATUS);

    ata_wait_bsy();
    
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LO, (unsigned char)(lba));
    outb(ATA_PRIMARY_LBA_MID, (unsigned char)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (unsigned char)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, 0x30);
    
    ata_wait_bsy();
    ata_wait_drq();
    
    // Veriyi 16-bit word olarak porta gönder
    for (int i = 0; i < 256; i++) {
        unsigned short word = (unsigned short)(buffer[i * 2]) | ((unsigned short)(buffer[i * 2 + 1]) << 8);
        outw(ATA_PRIMARY_DATA, word);
    }
    
    // Diskin yazmayı bitirmesi için FLUSH CACHE
    outb(ATA_PRIMARY_COMMAND, 0xE7);
    ata_wait_bsy();
    spin_unlock(&ata_lock);
}
