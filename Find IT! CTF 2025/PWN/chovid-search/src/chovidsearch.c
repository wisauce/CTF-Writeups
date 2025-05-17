#include "qemu/osdep.h"
#include "qemu/units.h"
#include "hw/pci/pci.h"
#include "hw/hw.h"
#include "hw/pci/msi.h"
#include "qom/object.h"
#include "qemu/module.h"
#include "qapi/visitor.h"
#include "sysemu/dma.h"

#define MAX_TEXT_LEN 1024
#define MAX_PATTERN_LEN 128

typedef struct ChovidSearchState {
    char text[MAX_TEXT_LEN];
    char pattern[MAX_PATTERN_LEN];
    uint16_t memo[MAX_PATTERN_LEN];
    char replacement[MAX_PATTERN_LEN];
    
    AddressSpace *as;
    MemoryRegion mmio;
    PCIDevice pdev;
    size_t text_len;
    size_t pattern_len;
    size_t replacement_len;
    bool pattern_exists;
} ChovidSearchState;

DECLARE_INSTANCE_CHECKER(ChovidSearchState, CHOVIDSEARCH, "chovidsearch")

#define MMIO_OFFSET_PATTERN_EXISTS 0x800
#define MMIO_TRIGGER_REPLACE 0x808
#define MMIO_TRIGGER_FIND 0x810

static void compute_lps(const char* pattern, uint16_t* memo, size_t pattern_len) {
    size_t len = 0;
    memo[0] = 0;
    size_t i = 1;
    
    while (i < pattern_len) {
        if (pattern[i] == pattern[len]) {
            len++;
            memo[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = memo[len - 1];
            } else {
                memo[i] = 0;
                i++;
            }
        }
    }
}

static void chovidsearch_replace(ChovidSearchState* s) {
    compute_lps(s->pattern, s->memo, s->pattern_len);
    
    int i = 0, j = 0;
    s->pattern_exists = false;
    
    while (i < s->text_len) {
        if (s->pattern[j] == s->text[i]) {
            i++;
            j++;
            if (j >= s->pattern_len) {
                for (size_t k = j - s->pattern_len; k < j; k++) {
                    s->pattern[k] = s->replacement[k - (j - s->pattern_len)];
                }
                memcpy(&s->text[i - s->pattern_len], s->pattern, s->pattern_len);
                break;
            }
        } else if (i < s->text_len) {
            j = s->memo[j - 1];
            if (j == 0) {
                i++;
            }
        }
    }
}

static void chovidsearch_find(ChovidSearchState* s) {
    compute_lps(s->pattern, s->memo, s->pattern_len);
    
    int i = 0, j = 0;
    s->pattern_exists = false;
    
    while (i < s->text_len) {
        if (s->pattern[j] == s->text[i]) {
            i++;
            j++;
            if (j >= s->pattern_len) {
                s->pattern_exists = true;
                break;
            }
        } else if (i < s->text_len) {
            j = s->memo[j - 1];
            if (j == 0) {
                i++;
            }
        }
    }
}

static uint64_t mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    ChovidSearchState *s = opaque;
    uint64_t val = 0;

    switch (addr) {
        case MMIO_OFFSET_PATTERN_EXISTS:
            val = s->pattern_exists;
            break;
    }

    return val;
}

static void mmio_write(void *opaque, hwaddr addr, uint64_t val,
                unsigned size)
{
    ChovidSearchState *s = opaque;

    if (addr < MAX_TEXT_LEN) {
        if (addr + 8 <= MAX_TEXT_LEN) {
            memcpy(s->text + addr, &val, 8);
            s->text_len = strlen(s->text) < MAX_TEXT_LEN ? strlen(s->text) : MAX_TEXT_LEN;
        }
    } else if (addr >= MAX_TEXT_LEN && addr < MAX_TEXT_LEN + MAX_PATTERN_LEN) {
        size_t pattern_offset = addr - MAX_TEXT_LEN;
        if (pattern_offset + 8 <= MAX_PATTERN_LEN) {
            memcpy(s->pattern + pattern_offset, &val, 8);
            s->pattern_len = strlen(s->pattern) < MAX_PATTERN_LEN ? strlen(s->pattern) : MAX_PATTERN_LEN;
        }
    } else if (addr >= MAX_TEXT_LEN + MAX_PATTERN_LEN && addr < MAX_TEXT_LEN + MAX_PATTERN_LEN + MAX_PATTERN_LEN) {
        size_t replacement_offset = addr - (MAX_TEXT_LEN + MAX_PATTERN_LEN);
        if (replacement_offset + 8 <= MAX_PATTERN_LEN) {
            memcpy(s->replacement + replacement_offset, &val, 8);
            s->replacement_len = strlen(s->replacement) < MAX_PATTERN_LEN ? strlen(s->replacement) : MAX_PATTERN_LEN;
        }
    } else {
        switch (addr) {
            case MMIO_TRIGGER_REPLACE:
                chovidsearch_replace(s);
                break;
            case MMIO_TRIGGER_FIND:
                chovidsearch_find(s);
                break;
        }
    }
}

static const MemoryRegionOps mmio_ops = {
    .read = mmio_read,
    .write = mmio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 8,
    },
    .impl = {
        .min_access_size = 4,
        .max_access_size = 8,
    },
};

static void realize(PCIDevice *pdev, Error **errp)
{
    ChovidSearchState *s = CHOVIDSEARCH(pdev);

    if (msi_init(pdev, 0, 1, true, false, errp)) {
        return;
    }

    s->as = &address_space_memory;
    memory_region_init_io(&s->mmio, OBJECT(s), &mmio_ops, s,
                    "chovidsearch-mmio", 1 * MiB);
    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);
}

static void uninit(PCIDevice *pdev)
{
    msi_uninit(pdev);
}

static void instance_init(Object *obj)
{
}

static void class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

    k->realize = realize;
    k->exit = uninit;
    k->vendor_id = 0xbabe;
    k->device_id = 0xbeef;
    k->revision = 0x45;
    k->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static void pci_register_types(void)
{
    static InterfaceInfo interfaces[] = {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    };

    static const TypeInfo info = {
        .name          = "chovidsearch",
        .parent        = TYPE_PCI_DEVICE,
        .instance_size = sizeof(ChovidSearchState),
        .instance_init = instance_init,
        .class_init    = class_init,
        .interfaces = interfaces,
    };

    type_register_static(&info);
}
type_init(pci_register_types)