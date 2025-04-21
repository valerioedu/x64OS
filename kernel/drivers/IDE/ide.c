#include "ide.h"
#include "../PCI/pci.h"
#include "../../cpu/src/pic.h"

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
    {ATA_PRIMARY, 0x3F6, 0},     /* will be overwritten if controller is in native mode */
    {ATA_SECONDARY, 0x376, 0}
};

static uint16_t lba_count = 129;        /*1 sector for bootloader, 128 for the first load*/

static ide_channel_status_t channel_status[2] = { {0}, {0} };

static int ata_get_channel_bases(void) {
    for (int i = 0; i < pci_get_device_count(); ++i) {
        const PciDevice *d = pci_get_device(i);
        if (d->class_code == 0x01 && d->subclass == 0x01) {
            uint32_t bar0 = pci_config_read32(d->bus, d->device, d->function, 0x10);
            uint32_t bar1 = pci_config_read32(d->bus, d->device, d->function, 0x14);

            uint16_t io_base   = 0x1F0;
            uint16_t ctrl_base = 0x3F6;

            if (bar0) {
                if (bar0 & 1) io_base = bar0 & 0xFFFC;
            }

            if (bar1) {
                if (bar1 & 1) ctrl_base = bar1 & 0xFFFC;
            }

            ide_channels[0].base         = io_base;
            ide_channels[0].control_base = ctrl_base;

            kprintf("IDE: base 0x%x  ctrl 0x%x (progIF 0x%x)\n",
                    ide_channels[0].base,
                    ide_channels[0].control_base,
                    d->prog_if);
            return 0;
        }
    }
    kprint("No PCI IDE controller found\n");
    return -1;
}

static int ata_wait(uint16_t io, int check_err) {
    for (int i = 0; i < 4; ++i) inb(io + ATA_REG_STATUS);

    for (uint32_t t = 0; t < 1000000; ++t) {
        uint8_t st = inb(io + ATA_REG_STATUS);
        if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ))
            return 0;
        if (check_err && (st & ATA_SR_ERR))
            return -1;
    }
    return -2;
}

void ide_wait(uint16_t io) { ata_wait(io, 0); }

static inline void ide_enable_irq(int channel) {
    outb(ide_channels[channel].control_base, 0);
}

void ide_init(void) {
    kprint("Initializing IDE…\n");

    if (ata_get_channel_bases() != 0) return;

    uint16_t io = ide_channels[0].base;

    outb(io + ATA_REG_DRIVE_SELECT, ATA_MASTER);
    ide_wait(io);

    outb(io + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ide_wait(io);

    uint8_t status = inb(io + ATA_REG_STATUS);
    if (status == 0) { kprint("No drive on primary master\n"); return; }
    if (status & ATA_SR_ERR) { kprint("ERR during IDENTIFY\n"); return; }
    if (ata_wait(io, 1) != 0) { kprint("IDENTIFY timed out\n"); return; }

    uint16_t id[256];
    for (int i = 0; i < 256; ++i) id[i] = inw(io + ATA_REG_DATA);

    char model[41];
    for (int i = 0; i < 40; i += 2) {
        model[i]     = (id[27 + i/2] >> 8) & 0xFF;
        model[i + 1] =  id[27 + i/2]       & 0xFF;
    }
    model[40] = '\0';
    kprintf("Primary master detected: %s\n", model);

    ide_enable_irq(0);
    kprint("IDE initialization complete.\n");
}

static int ide_pio_28(uint8_t drive, uint32_t lba,
                      uint8_t cmd, uint16_t sectors,
                      const uint16_t *wbuf, uint16_t *rbuf)
{
    int      channel   = drive & 1;
    int      ata_drive = (drive & 2) >> 1;
    uint16_t io        = ide_channels[channel].base;

    int w = ata_wait(io, 1);

    outb(io + ATA_REG_SECTOR_COUNT, (uint8_t)sectors);
    outb(io + ATA_REG_LBA_LOW, (uint8_t)  lba);
    outb(io + ATA_REG_LBA_MID, (uint8_t)( lba >> 8));
    outb(io + ATA_REG_LBA_HIGH, (uint8_t)( lba >>16));
    outb(io + ATA_REG_DRIVE_SELECT, 0xE0 | (ata_drive << 4) | ((lba >> 24) & 0x0F));

    outb(io + ATA_REG_COMMAND, cmd);

    for (uint16_t s = 0; s < sectors; ++s) {
        if (ata_wait(io, 1) != 0) return 0;

        if (cmd == ATA_CMD_READ_PIO) {
            for (int i = 0; i < 256; ++i) *rbuf++ = inw(io + ATA_REG_DATA);
        } else {
            for (int i = 0; i < 256; ++i) outw(io + ATA_REG_DATA, *wbuf++);
        }
    }

    if (cmd == ATA_CMD_WRITE_PIO) {
        ata_wait(io, 0);
        outb(io + ATA_REG_COMMAND, 0xE7);
    }
    return 1;
}

int ide_read_sectors (uint8_t d, uint32_t l, uint16_t c, void *b)
{ return ide_pio_28(d, l, ATA_CMD_READ_PIO,  c, NULL, b); }

int ide_write_sectors(uint8_t d, uint32_t l, uint16_t c, const void *b)
{ return ide_pio_28(d, l, ATA_CMD_WRITE_PIO, c, b, NULL); }

int ide_read (uint8_t d, uint32_t l, uint8_t c, uint16_t *b)
{ return ide_read_sectors (d, l, c, b); }

int ide_write(uint8_t d, uint32_t l, uint8_t c, uint16_t *b)
{ return ide_write_sectors(d, l, c, b); }

void ide_handle_interrupt(int channel) {
    uint16_t io = ide_channels[channel].base;
    ide_channel_status_t *s = &channel_status[channel];

    if (!s->busy) goto eoi;

    uint8_t st = inb(io + ATA_REG_STATUS);

    if (st & ATA_SR_ERR) {
        s->busy = 0;
        s->callback_ready = 1;
        goto eoi;
    }

    if (s->command == ATA_CMD_READ_PIO && (st & ATA_SR_DRQ)) {
        int words = (s->words_left < 256) ? s->words_left : 256;

        for (int i = 0; i < words; ++i) *s->buffer++ = inw(io + ATA_REG_DATA);

        s->words_left   -= words;
        if (s->words_left == 0) {
            s->sectors_left--;
            if (s->sectors_left) {
                s->words_left = 256;
            } else {
                s->busy = 0;
                s->callback_ready = 1;
            }
        }
    }

    else if (s->command == ATA_CMD_WRITE_PIO && (st & ATA_SR_DRQ)) {
        int words = (s->words_left < 256) ? s->words_left : 256;

        for (int i = 0; i < words; ++i) outw(io + ATA_REG_DATA, *s->buffer++);

        s->words_left   -= words;
        if (s->words_left == 0) {
            s->sectors_left--;
            if (s->sectors_left) {
                s->words_left = 256;
            } else {
                outb(io + ATA_REG_COMMAND, 0xE7);
                s->command = 0;
            }
        }
    }

eoi:
    pic_send_eoi(14 + channel);
}

uint16_t ide_get_lba_count(void) 
{ return lba_count; }

int load_sectors(uint8_t drive, uint16_t count, uint16_t* buffer) {
    uint16_t lba = ide_get_lba_count();
    uint16_t sectors_read = 0;
    int result = -count;

    while (sectors_read < count) {
        uint16_t sectors = (count - sectors_read > 255) ? 255 : (count - sectors_read);

        result += ide_read_sectors(drive, lba + sectors_read, sectors, buffer + sectors_read * 256);
        sectors_read += sectors;
    }

    lba_count += count;

    if (result < 0) {
        kprintf("Error reading sectors: %d\n", result);
        return -1;
    }

    return 1;
}