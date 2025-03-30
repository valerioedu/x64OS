#include "ide.h"

typedef struct {
    int busy;
    int drive;
    uint8_t command;
    uint32_t lba;
    uint8_t sector_count;
    uint16_t* buffer;
    uint16_t words_left;
    uint8_t sectors_left;
    int callback_ready;
} ide_channel_status_t;

IDEChannel ide_channels[2] = {
    {ATA_PRIMARY, 0x3F6, 0},
    {ATA_SECONDARY, 0x376, 0}
};

ide_channel_status_t channel_status[2] = { {0}, {0} };

void ide_wait(uint16_t io) {
    for (int i = 0; i < 4; i++) {
        inb(io + ATA_REG_STATUS);
    }
}

void ide_enable_irq(int channel) {
    if (channel == 0) {
        outb(ide_channels[0].control_base, 0);
    } else {
        outb(ide_channels[1].control_base, 0);
    }
}

void ide_init() {
    kprint("Initializing IDE\n");

    outb(ATA_PRIMARY + ATA_REG_DRIVE_SELECT, ATA_MASTER);
    ide_wait(ATA_PRIMARY);

    outb(ATA_PRIMARY + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ide_wait(ATA_PRIMARY);

    if (inb(ATA_PRIMARY + ATA_REG_STATUS) == 0) {
        kprint("No IDE drive found on primary channel\n");
    } else {
        kprint("Primary master drive detected\n");
        
        uint16_t identify_data[256];
        for (int i = 0; i < 256; i++) {
            identify_data[i] = inw(ATA_PRIMARY + ATA_REG_DATA);
        }
        
        char model[41];
        for (int i = 0; i < 40; i += 2) {
            model[i] = (identify_data[27 + i/2] >> 8) & 0xFF;
            model[i+1] = identify_data[27 + i/2] & 0xFF;
        }
        model[40] = '\0';
        
        kprintf("Drive model: %s\n", model);
    }

    outb(ATA_SECONDARY + ATA_REG_DRIVE_SELECT, ATA_MASTER);
    ide_wait(ATA_SECONDARY);

    outb(ATA_SECONDARY + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ide_wait(ATA_SECONDARY);

    if (inb(ATA_SECONDARY + ATA_REG_STATUS) == 0) {
        kprint("No IDE drive found on secondary channel\n");
    } else {
        kprint("Secondary master drive detected\n");
    }

    ide_enable_irq(0);
    ide_enable_irq(1);

    kprint("IDE initialization complete\n");
}

void ide_handle_interrupt(int channel) {
    uint16_t base = ide_channels[channel].base;
    ide_channel_status_t* status = &channel_status[channel];
    
    if (!status->busy) {
        //kprintf("Unexpected IDE IRQ on channel %d\n", channel);
        return;
    }
    
    uint8_t status_reg = inb(base + ATA_REG_STATUS);
    
    if (status_reg & ATA_SR_ERR) {
        //kprintf("IDE Error on channel %d: 0x%x\n", channel, inb(base + ATA_REG_ERROR));
        status->busy = 0;
        status->callback_ready = 1;
        return;
    }
    
    if (status->command == ATA_CMD_READ_PIO) {
        for (int i = 0; i < 256 && status->words_left > 0; i++) {
            *status->buffer++ = inw(base + ATA_REG_DATA);
            status->words_left--;
        }
    }
    
    if (status->words_left == 0) {
        status->sectors_left--;
        
        if (status->sectors_left > 0) {
            status->words_left = 256;
        } else {
            status->busy = 0;
            status->callback_ready = 1;
        }
    }
}

int ide_read_async(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    int channel = (drive & 1);
    int ata_drive = (drive & 2) >> 1;
    uint16_t base = ide_channels[channel].base;
    
    if (channel_status[channel].busy) {
        return 0;
    }
    
    channel_status[channel].busy = 1;
    channel_status[channel].drive = ata_drive;
    channel_status[channel].command = ATA_CMD_READ_PIO;
    channel_status[channel].lba = lba;
    channel_status[channel].sector_count = sector_count;
    channel_status[channel].buffer = buffer;
    channel_status[channel].words_left = 256;
    channel_status[channel].sectors_left = sector_count;
    channel_status[channel].callback_ready = 0;
    
    while (inb(base + ATA_REG_STATUS) & ATA_SR_BSY);
    
    outb(base + ATA_REG_DRIVE_SELECT, 0xE0 | (ata_drive << 4) | ((lba >> 24) & 0x0F));
    outb(base + ATA_REG_SECTOR_COUNT, sector_count);
    outb(base + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    return 1;
}

int ide_write_async(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    int channel = (drive & 1);
    int ata_drive = (drive & 2) >> 1;
    uint16_t base = ide_channels[channel].base;
    
    if (channel_status[channel].busy) {
        return 0;
    }
    
    channel_status[channel].busy = 1;
    channel_status[channel].drive = ata_drive;
    channel_status[channel].command = ATA_CMD_WRITE_PIO;
    channel_status[channel].lba = lba;
    channel_status[channel].sector_count = sector_count;
    channel_status[channel].buffer = buffer;
    channel_status[channel].words_left = 256;
    channel_status[channel].sectors_left = sector_count;
    channel_status[channel].callback_ready = 0;
    
    while (inb(base + ATA_REG_STATUS) & ATA_SR_BSY);
    
    outb(base + ATA_REG_DRIVE_SELECT, 0xE0 | (ata_drive << 4) | ((lba >> 24) & 0x0F));
    outb(base + ATA_REG_SECTOR_COUNT, sector_count);
    outb(base + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    
    outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    while (!(inb(base + ATA_REG_STATUS) & ATA_SR_DRQ));
    
    for (int i = 0; i < 256; i++) {
        outw(base + ATA_REG_DATA, buffer[i]);
    }
    
    channel_status[channel].buffer += 256;
    channel_status[channel].sectors_left--;
    
    if (channel_status[channel].sectors_left == 0) {
        channel_status[channel].busy = 0;
        channel_status[channel].callback_ready = 1;
    }
    
    return 1;
}

int ide_read(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    if (!ide_read_async(drive, lba, sector_count, buffer)) {
        return 0;
    }
    
    int channel = (drive & 1);
    
    while (!channel_status[channel].callback_ready) {
        asm volatile("pause");
    }
    
    return 1;
}

int ide_write(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    if (!ide_write_async(drive, lba, sector_count, buffer)) {
        return 0;
    }
    
    int channel = (drive & 1);
    
    while (!channel_status[channel].callback_ready) {
        asm volatile("pause");
    }
    
    return 1;
}

int ide_operation_complete(uint8_t drive) {
    int channel = (drive & 1);
    return channel_status[channel].callback_ready;
}