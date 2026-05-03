#include "ata.h"

extern void outb(unsigned short port, unsigned char val);
extern unsigned char inb(unsigned short port);
extern void outw(unsigned short port, unsigned short val);
extern unsigned short inw(unsigned short port);

// BSY (Busy) bitinin 0 olmasını bekle
void ata_wait_bsy(void) {
    while (inb(ATA_PRIMARY_STATUS) & 0x80);
}

// DRQ (Data Request) bitinin 1 olmasını bekle
void ata_wait_drq(void) {
    while (!(inb(ATA_PRIMARY_STATUS) & 0x08));
}

// LBA28 modunda 1 sektör (512 byte) oku
void ata_read_sector(unsigned int lba, unsigned char *buffer) {
    ata_wait_bsy();
    
    outb(ATA_PRIMARY_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F)); // Master drive + LBA bit 24-27
    outb(ATA_PRIMARY_SECCOUNT, 1);                           // Read 1 sector
    outb(ATA_PRIMARY_LBA_LO, (unsigned char)(lba));
    outb(ATA_PRIMARY_LBA_MID, (unsigned char)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (unsigned char)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, 0x20);                         // Command: Read Sector
    
    ata_wait_bsy();
    ata_wait_drq();
    
    // Veriyi 16-bit word olarak porttan al
    for (int i = 0; i < 256; i++) {
        unsigned short word = inw(ATA_PRIMARY_DATA);
        buffer[i * 2] = (unsigned char)(word & 0xFF);
        buffer[i * 2 + 1] = (unsigned char)((word >> 8) & 0xFF);
    }
}

// LBA28 modunda 1 sektör (512 byte) yaz
void ata_write_sector(unsigned int lba, unsigned char *buffer) {
    ata_wait_bsy();
    
    outb(ATA_PRIMARY_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LO, (unsigned char)(lba));
    outb(ATA_PRIMARY_LBA_MID, (unsigned char)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (unsigned char)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, 0x30);                         // Command: Write Sector
    
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
}
