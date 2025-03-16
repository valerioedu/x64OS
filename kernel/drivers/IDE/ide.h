#ifndef IDE_H
#define IDE_H

#include "../../../lib/definitions.h"

#define ATA_PRIMARY 0x1F0
#define ATA_SECONDARY 0x170
#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0

#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_FEATURES    0x01
#define ATA_REG_SECTOR_COUNT 0x02
#define ATA_REG_LBA_LOW     0x03
#define ATA_REG_LBA_MID     0x04
#define ATA_REG_LBA_HIGH    0x05
#define ATA_REG_DRIVE_SELECT 0x06
#define ATA_REG_COMMAND     0x07
#define ATA_REG_STATUS      0x07

#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30

#define ATA_SR_BSY  0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF   0x20
#define ATA_SR_ERR  0x01

typedef struct {
    uint16_t base;
    uint16_t ctrl;
    uint8_t slave;
} IDEChannel;

void ide_init();
int ide_read(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer);
int ide_write(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer);

#endif