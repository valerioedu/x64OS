#include "ide.h"

IDEChannel ide_channels[2] = {
    {ATA_PRIMARY, 0x3F6, 0},
    {ATA_SECONDARY, 0x376, 0}
};

void ide_wait(uint16_t io) {
    for (int i = 0; i < 4; i++) {
        inb(io + ATA_REG_STATUS);
    }
}

void ide_init() {
    kprint("Initializing IDE\n");

    outb(ATA_PRIMARY + ATA_REG_DRIVE_SELECT, ATA_MASTER);
    ide_wait(ATA_PRIMARY);

    outb(ATA_PRIMARY + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ide_wait(ATA_PRIMARY);

    if (inb(ATA_PRIMARY + ATA_REG_STATUS) == 0) {
        kprint("No IDE drive found\n");
        return;
    }

    kprint("IDE initialized\n");
}

int ide_read(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    uint16_t base = (drive == 0) ? ATA_PRIMARY : ATA_SECONDARY;

    while (inb(base + ATA_REG_STATUS) & ATA_SR_BSY);

    outb(base + ATA_REG_DRIVE_SELECT, 0xE0 | ((lba >> 24) & 0x0F));
    outb(base + ATA_REG_SECTOR_COUNT, sector_count);
    outb(base + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    for (int i = 0; i < sector_count; i++) {
        while (!(inb(base + ATA_REG_STATUS) & ATA_SR_DRDY));

        for (int j = 0; j < 256; j++) {
            buffer[j] = inw(base + ATA_REG_DATA);
        }
    }

    return 1;
}

int ide_write(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    uint16_t base = (drive == 0) ? ATA_PRIMARY : ATA_SECONDARY;

    while (inb(base + ATA_REG_STATUS) & ATA_SR_BSY);

    outb(base + ATA_REG_DRIVE_SELECT, 0xE0 | ((lba >> 24) & 0x0F));
    outb(base + ATA_REG_SECTOR_COUNT, sector_count);
    outb(base + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    for (int i = 0; i < sector_count; i++) {
        while (!(inb(base + ATA_REG_STATUS) & ATA_SR_DRDY));

        for (int j = 0; j < 256; j++) {
            outw(base + ATA_REG_DATA, buffer[j]);
        }
    }

    return 1;
}