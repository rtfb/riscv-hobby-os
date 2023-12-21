#ifndef _PMP_H
#define _PMP_H

#ifdef TARGET_M_MODE
#define HAS_PMP  1
#else
#undef HAS_PMP
#endif

#define PAGE_SIZE           512 // bytes

// Let's hardcode it for now. Make it small enough to fit in HiFive1 (32 pages
// * 512 bytes = 16k RAM).
#define MAX_PAGES           32

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

typedef struct pmp_config_s {
#if HAS_PMP
    uint8_t pmpxx[NUM_PMP_CSRS];    // stores pmpXXcfg bytes
#endif
    int num_available;              // number of configs available for dynamic use
} pmp_config_t;

// defined in boot.s:
extern void* user_payload;
extern void* rodata;

// defined in kernel.ld:
extern void* stack_bottom;
extern void* stack_top;

// Returns the end of RAM address, which is then passed to init_paged_memory.
void* init_pmp();
void* shift_right_addr(void* addr, int bits);
void set_page_pmp_perms(pmp_config_t *config, void *page, int index, int perms);
void set_page_pmp_perms2(pmp_config_t *config, void *page, int index, int perms);
void reset_page_pmp_perms(pmp_config_t *config, int index);
void set_pmpaddr_idx(int index, void *addr);
int find_free_pmp_slot(pmp_config_t *config);

void init_pmp_config(pmp_config_t *config, void* paged_mem_start);
void apply_pmp_config(pmp_config_t *config);
void apply_ith_config(pmp_config_t *config, int i);
void* adjust_pmp_napot_addr(void *addr);

#endif // ifndef _PMP_H
