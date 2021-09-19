#ifndef _FDT_H_
#define _FDT_H_

#include "sys.h"

// FDT header signature.
#define FDT_MAGIC       0xd00dfeed

// This is implemented against version 17 of the devicetree spec.
#define FDT_VERSION     17

#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

typedef struct fdt_header_s {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dr_strings;
    uint32_t size_dt_struct;
} fdt_header;

void fdt_init(uintptr_t header_addr);
char const* fdt_get_bootargs();

#endif // ifndef _FDT_H_
