#include "pci.h"

static PciDevice pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

static uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    uint32_t address = (1u << 31)            |
                       ((uint32_t)bus      << 16) |
                       ((uint32_t)device    << 11) |
                       ((uint32_t)function  <<  8) |
                       (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static void pci_config_write32(uint8_t bus, uint8_t device, uint8_t function,
                               uint8_t offset, uint32_t value)
{
    uint32_t address = (1u << 31)            |
                       ((uint32_t)bus      << 16) |
                       ((uint32_t)device    << 11) |
                       ((uint32_t)function  <<  8) |
                       (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

static uint16_t pci_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t val = pci_config_read32(bus, device, function, offset & 0xFC);
    return (val >> ((offset & 2) * 8)) & 0xFFFF;
}

static uint8_t pci_config_read8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t val = pci_config_read32(bus, device, function, offset & 0xFC);
    return (val >> ((offset & 3) * 8)) & 0xFF;
}

static void pci_add_device(uint8_t bus, uint8_t dev, uint8_t func) {
    if (pci_device_count >= MAX_PCI_DEVICES) return;

    PciDevice* d = &pci_devices[pci_device_count++];
    d->bus      = bus;
    d->device   = dev;
    d->function = func;

    d->vendor_id = pci_config_read16(bus, dev, func, 0x00);
    if (d->vendor_id == 0xFFFF) {
        pci_device_count--;
        return;
    }
    d->device_id = pci_config_read16(bus, dev, func, 0x02);

    uint32_t classReg = pci_config_read32(bus, dev, func, 0x08);
    d->revision  = classReg & 0xFF;
    d->prog_if   = (classReg >> 8) & 0xFF;
    d->subclass  = (classReg >> 16) & 0xFF;
    d->class_code= (classReg >> 24) & 0xFF;

    kprintf("Found PCI device: Bus %d, Device %d, Function %d\n", bus, dev, func);
    kprintf("Vendor ID: 0x%x, Device ID: 0x%x\n", d->vendor_id, d->device_id);
    kprintf("Class Code: 0x%X, Subclass: 0x%x, Prog IF: 0x%x, Revision: 0x%x\n",
            d->class_code, d->subclass, d->prog_if, d->revision);
    kprintf("PCI device %d added\n", pci_device_count);
}

static void pci_scan_function(uint8_t bus, uint8_t dev, uint8_t func) {
    pci_add_device(bus, dev, func);
}

static void pci_scan_device(uint8_t bus, uint8_t dev) {
    uint16_t vendor = pci_config_read16(bus, dev, 0, 0x00);
    if (vendor == 0xFFFF) return;

    uint8_t header = pci_config_read8(bus, dev, 0, 0x0E);
    uint8_t multifunction = header & 0x80;

    for (uint8_t func = 0; func < (multifunction ? PCI_MAX_FUNCTION : 1); ++func) {
        vendor = pci_config_read16(bus, dev, func, 0x00);
        if (vendor != 0xFFFF) {
            pci_scan_function(bus, dev, func);
        }
    }
}

static void pci_scan_bus(uint8_t bus) {
    for (uint8_t dev = 0; dev < PCI_MAX_DEVICE; ++dev) {
        pci_scan_device(bus, dev);
    }
}

void pci_init(void) {
    kprint("Scanning PCI buses...\n");

    uint8_t header_type = pci_config_read8(0, 0, 0, 0x0E);
    if ((header_type & 0x80) == 0) {
        pci_scan_bus(0);
    } else {
        for (uint8_t func = 0; func < PCI_MAX_FUNCTION; ++func) {
            if (pci_config_read16(0, 0, func, 0x00) == 0xFFFF) continue;
            uint8_t secondary_bus = pci_config_read8(0, 0, func, 0x19);
            pci_scan_bus(secondary_bus);
        }
    }

    kprintf("PCI scan complete. Found %d devices.\n", pci_device_count);
}

int pci_get_device_count(void) {return pci_device_count;}

const PciDevice* pci_get_device(int index) {
    return (index < pci_device_count) ? &pci_devices[index] : NULL;
}