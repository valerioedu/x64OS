#ifndef PCI_H
#define PCI_H

#include "../../../lib/definitions.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_MAX_BUS       256
#define PCI_MAX_DEVICE     32
#define PCI_MAX_FUNCTION    8

#define MAX_PCI_DEVICES 256

typedef struct PciDevice {
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;

    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  revision;
} PciDevice;

void pci_init(void);
int pci_get_device_count(void);
const PciDevice* pci_get_device(int index);

#endif