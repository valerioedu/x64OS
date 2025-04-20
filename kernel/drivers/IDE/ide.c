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

static int ata_wait(uint16_t io, int check_err) {
    for (int i = 0; i < 4; i++) inb(io + ATA_REG_STATUS);

    for (uint32_t t = 0;t < 1000000; t++) {
        uint8_t st = inb(io + ATA_REG_STATUS);
        if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ)) return 0;
        if (check_err && (st & ATA_SR_ERR)) return -1;
    }
    return -2;
}

void ide_wait(uint16_t io) {
    ata_wait(io, 0);
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

static int ide_pio_28(uint8_t drive, uint32_t lba,
                      uint8_t cmd, uint16_t sectors,
                      const uint16_t *wbuf,
                      uint16_t *rbuf)
{
    int channel   = drive & 1;
    int ata_drive = (drive & 2) >> 1;
    uint16_t io   = ide_channels[channel].base;

    while (inb(io + ATA_REG_STATUS) & ATA_SR_BSY) ;

    outb(io + ATA_REG_DRIVE_SELECT, 0xE0 | (ata_drive << 4) | ((lba >> 24) & 0x0F));
    outb(io + ATA_REG_SECTOR_COUNT, sectors);
    outb(io + ATA_REG_LBA_LOW,      (uint8_t) lba);
    outb(io + ATA_REG_LBA_MID,      (uint8_t)(lba >> 8));
    outb(io + ATA_REG_LBA_HIGH,     (uint8_t)(lba >> 16));

    outb(io + ATA_REG_COMMAND, cmd);

    for (uint16_t s = 0; s < sectors; ++s) {
        uint8_t st;

        if (st & ATA_SR_ERR) return 0;

        int r = ata_wait(io, 1);
        if (r) return 0;
        kprint("ok\n");

        if (cmd == ATA_CMD_READ_PIO) {
            for (int i = 0; i < 256; ++i) *rbuf++ = inw(io + ATA_REG_DATA);
        } else {
            for (int i = 0; i < 256; ++i) outw(io + ATA_REG_DATA, *wbuf++);
        }
    }

    if (cmd == ATA_CMD_WRITE_PIO) {
        while (inb(io + ATA_REG_STATUS) & ATA_SR_BSY) ;
            outb(io + ATA_REG_COMMAND, 0xE7);
    }
    return 1;
}

int ide_read_sectors(uint8_t drive, uint32_t lba, uint16_t count, void *buf) {
    return ide_pio_28(drive, lba, ATA_CMD_READ_PIO, count, NULL, buf);
}

int ide_write_sectors(uint8_t drive, uint32_t lba, uint16_t count, const void *buf) {
    return ide_pio_28(drive, lba, ATA_CMD_WRITE_PIO, count, buf, NULL);
}

int ide_read (uint8_t d, uint32_t l, uint8_t c, uint16_t *b){return ide_read_sectors (d,l,c,b);}
int ide_write(uint8_t d, uint32_t l, uint8_t c, uint16_t *b){return ide_write_sectors(d,l,c,b);}