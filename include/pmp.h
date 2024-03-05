#ifndef _PMP_H
#define _PMP_H

#define PMP_LOCK  (1 << 7)
#define PMP_NAPOT (3 << 3)          // Address Mode: Naturally aligned power-of-two region, >=8 bytes
#define PMP_NA4   (2 << 3)          // Address Mode: Naturally aligned four-byte region
#define PMP_TOR   (1 << 3)          // Address Mode: Top of range, address register forms the top of the address range,

#define PMP_X     (1 << 2)          // Access Mode: Executable
#define PMP_W     (1 << 1)          // Access Mode: Writable
#define PMP_R     (1 << 0)          // Access Mode: Readable
#define PMP_0     (1 << 0)          // [0..8]   1st PMP entry in pmpcfgX register
#define PMP_1     (1 << 8)          // [8..16]  2nd PMP entry in pmpcfgX register
#define PMP_2     (1 << 16)         // [16..24] 3rd PMP entry in pmpcfgX register
#define PMP_3     (1 << 24)         // [24..32] 4th PMP entry in pmpcfgX register

// defined in boot.s:
extern void* user_payload;
extern void* rodata;

// defined in kernel.ld:
extern void* stack_bottom_addr;
extern void* stack_top_addr;

// Returns the end of RAM address, which is then passed to init_paged_memory.
void* init_pmp();

#endif // ifndef _PMP_H
