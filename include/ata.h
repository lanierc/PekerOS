#ifndef ATA_H
#define ATA_H

#include "common.h"

// Primary ATA Bus I/O Ports
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRV_HEAD     0x1F6
#define ATA_PRIMARY_STATUS       0x1F7
#define ATA_PRIMARY_COMMAND      0x1F7

void ata_wait_bsy(void);
void ata_wait_drq(void);
void ata_read_sector(unsigned char drive, unsigned int lba, unsigned char *buffer);
void ata_write_sector(unsigned char drive, unsigned int lba, unsigned char *buffer);

#endif
