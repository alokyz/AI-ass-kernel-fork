#ifndef ORANGEX_ATA_H
#define ORANGEX_ATA_H

#include "types.h"

#define ATA_PRIMARY   0x1F0
#define ATA_SECONDARY 0x170

void     ata_init(void);
uint8_t* ata_read_sectors(uint32_t lba, uint32_t count);
int      ata_write_sectors(uint32_t lba, uint32_t count, uint8_t* data);
int      ata_detect(void);

#endif
