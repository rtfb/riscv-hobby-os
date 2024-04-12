// Rudimentary implementation of the Flattened Device Tree reader as specified
// in the Devicetree spec v0.3: https://www.devicetree.org/specifications/
//
// We're currently only interested in the bootargs passed on qemu command line
// via -append flag, and we take daring shortcuts to read it.

#include "fdt.h"
#include "kernel.h"
#include "string.h"

#define NODE_CHOSEN "chosen"
#define PROP_BOOTARGS "bootargs"

char bootargs[128];

uint32_t bswap(uint32_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t y = (x & 0x00ff00ff) << 8 | (x & 0xff00ff00) >> 8;
    uint32_t z = (y & 0x0000ffff) << 16 | (y & 0xffff0000) >> 16;
    return z;
#else
    return x
#endif
}

// reallign a pointer to the nearest greater multiple of 4
uint32_t* upalign4(void *x) {
    uintptr_t addr = (uintptr_t)x;
    addr = (addr + 3) & (-4);
    return (uint32_t*)addr;
}

char const* fdt_get_bootargs() {
    return bootargs;
}

void fdt_parse(uint32_t *tree, char const *strings) {
    uint32_t token = bswap(*tree);
    if (token != FDT_BEGIN_NODE) {
        return;
    }
    int found_chosen = 0;
    while (token != FDT_END) {
        while (token != FDT_BEGIN_NODE) {
            switch (token) {
                case FDT_PROP:
                    tree++;
                    uint32_t len = bswap(*tree++);
                    uint32_t name_offset = bswap(*tree++);
                    if (found_chosen) {
                        if (strncmp(strings+name_offset, PROP_BOOTARGS, ARRAY_LENGTH(PROP_BOOTARGS)) == 0) {
                            char const *arg = (char const*)tree;
                            strncpy(bootargs, arg, ARRAY_LENGTH(bootargs));
                            return;
                        }
                    }
                    tree = (uint32_t*)(((uintptr_t)tree) + len);
                    tree = upalign4(tree);
                    break;
                case FDT_NOP:
                case FDT_END_NODE:
                    tree++; // found_chosen = 0;
                    break;
                case FDT_END:
                    return;
            }
            token = bswap(*tree);
        }

        tree++;
        char *name = (char*)tree;
        char *end_of_name = name;
        while (*end_of_name++) {}
        tree = upalign4(end_of_name);
        token = bswap(*tree);
        // see if it's the "/chosen" node:
        if (strncmp(name, NODE_CHOSEN, ARRAY_LENGTH(NODE_CHOSEN)) == 0) {
            found_chosen = 1;
            continue;
        }
    }
}

void fdt_init(uintptr_t header_addr) {
    kprintf("Reading FDT...\n");
    fdt_header *header = (fdt_header*)header_addr;
    if (!header) {
        kprintf("FDT is NULL, nothing to do\n");
        return;
    }
    if (bswap(header->magic) != FDT_MAGIC) {
        // TODO: change that into a return value
        kprintf("bad FDT magic: %x\n", header->magic);
        return;
    }
    if (bswap(header->last_comp_version) > FDT_VERSION) {
        kprintf("unknown FDT version: %x\n", header->version);
        return;
    }
    uint32_t *tree = (uint32_t*)(header_addr + bswap(header->off_dt_struct));
    char const* strings = (char const*)(header_addr + bswap(header->off_dt_strings));
    fdt_parse(tree, strings);
    kprintf("FDT ok\n");
}
