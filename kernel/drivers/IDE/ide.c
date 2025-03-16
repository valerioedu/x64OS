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

}

int ide_write(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    
}