#include "kernel.h"

// 3.6.1 Physical Memory Protection CSRs
// > PMP entries are described by an 8-bit configuration register AND one XLEN-bit address register.
// > The PMP configuration registers are densely packed into CSRs to minimize context-switch time.
// >
// > For RV32, pmpcfg0..pmpcfg3, hold the configurations pmp0cfg..pmp15cfg for the 16 PMP entries.
// > For RV64, pmpcfg0 and pmpcfg2 hold the configurations for the 16 PMP entries; pmpcfg1 and pmpcfg3 are illegal.
// > For RV32, each PMP address register encodes bits 33:2 of a 34-bit physical address.
// > For RV64, each PMP address register encodes bits 55:2 of a 56-bit physical address.
// >
// > The R, W, and X bits, when set, indicate that the PMP entry permits read, write, and instruction execution.
// >
// > If PMP entry i's A field is set to TOR, the associated address register forms the top of the address range,
// > the entry matches any address a such that pmpaddr[i-1] <= a < pmpaddr[i]. If PMP entry 0's A field
// > is set to TOR, zero is used for the lower bound, and so it matches any address a < pmpaddr0.
// >
// > PMP entries are statically prioritized. The lowest-numbered PMP entry that matches any byte
// > of an access determines whether that access succeeds or fails.

// defined in kernel.ld:
extern void* RAM_START;
extern void* RAM_SIZE;

void* init_pmp() {
    // Calculate where is the end of our RAM:
    regsize_t ram_start = (regsize_t)&RAM_START;
    regsize_t ram_size = (regsize_t)&RAM_SIZE;
    void* paged_mem_end = (void*)(ram_start + ram_size);

    // Note: the rest of init is done in init_pmp_config() which is called later

    return paged_mem_end;
}

void* shift_right_addr(void* addr, int bits) {
    unsigned long iaddr = (unsigned long)addr;
    return (void*)(iaddr >> bits);
}

void set_page_pmp_perms(pmp_config_t *config, void *page, int index, int perms) {
#ifdef HAS_PMP
    set_pmpaddr_idx(index, adjust_pmp_napot_addr(page));
    config->pmpxx[index] = PMP_NAPOT | perms;
    apply_pmp_config(config);
#endif
}

void set_page_pmp_perms2(pmp_config_t *config, void *page, int index, int perms) {
#ifdef HAS_PMP
    set_pmpaddr_idx(index, (void*)(((regsize_t)page >> 2) & ~1));
    config->pmpxx[index] = PMP_NAPOT | perms;
    apply_pmp_config(config);
#endif
}

void reset_page_pmp_perms(pmp_config_t *config, int index) {
#ifdef HAS_PMP
    set_pmpaddr_idx(index, 0);
    config->pmpxx[index] = 0;
    apply_pmp_config(config);
#endif
}

int find_free_pmp_slot(pmp_config_t *config) {
#ifdef HAS_PMP
    for (int i = 0; i < config->num_available; i++) {
        if (config->pmpxx[i] == 0) {
            return i;
        }
    }
#endif
    return -1;
}

void set_pmpaddr_idx(int index, void *addr) {
#ifdef HAS_PMP
    switch (index) {
        case  0: set_pmpaddr(PMP_ADDR0_CSR, addr); break;
        case  1: set_pmpaddr(PMP_ADDR1_CSR, addr); break;
        case  2: set_pmpaddr(PMP_ADDR2_CSR, addr); break;
        case  3: set_pmpaddr(PMP_ADDR3_CSR, addr); break;
        case  4: set_pmpaddr(PMP_ADDR4_CSR, addr); break;
        case  5: set_pmpaddr(PMP_ADDR5_CSR, addr); break;
        case  6: set_pmpaddr(PMP_ADDR6_CSR, addr); break;
        case  7: set_pmpaddr(PMP_ADDR7_CSR, addr); break;
        case  8: set_pmpaddr(PMP_ADDR8_CSR, addr); break;
        case  9: set_pmpaddr(PMP_ADDR9_CSR, addr); break;
        case 10: set_pmpaddr(PMP_ADDR10_CSR, addr); break;
        case 11: set_pmpaddr(PMP_ADDR11_CSR, addr); break;
        case 12: set_pmpaddr(PMP_ADDR12_CSR, addr); break;
        case 13: set_pmpaddr(PMP_ADDR13_CSR, addr); break;
        case 14: set_pmpaddr(PMP_ADDR14_CSR, addr); break;
        case 15: set_pmpaddr(PMP_ADDR15_CSR, addr); break;
    }
#endif
}

void init_pmp_config(pmp_config_t *config, void* paged_mem_start) {
#if HAS_PMP
    // init all pmpaddrXX to 0. Can't loop over them because the CSR argument
    // must be a compile-time const
    set_pmpaddr(PMP_ADDR0_CSR, 0);
    set_pmpaddr(PMP_ADDR1_CSR, 0);
    set_pmpaddr(PMP_ADDR2_CSR, 0);
    set_pmpaddr(PMP_ADDR3_CSR, 0);
    set_pmpaddr(PMP_ADDR4_CSR, 0);
    set_pmpaddr(PMP_ADDR5_CSR, 0);
    set_pmpaddr(PMP_ADDR6_CSR, 0);
    set_pmpaddr(PMP_ADDR7_CSR, 0);
    set_pmpaddr(PMP_ADDR8_CSR, 0);
    set_pmpaddr(PMP_ADDR9_CSR, 0);
    set_pmpaddr(PMP_ADDR10_CSR, 0);
    set_pmpaddr(PMP_ADDR11_CSR, 0);
    set_pmpaddr(PMP_ADDR12_CSR, 0);
    set_pmpaddr(PMP_ADDR13_CSR, 0);
    set_pmpaddr(PMP_ADDR14_CSR, 0);
    set_pmpaddr(PMP_ADDR15_CSR, 0);

    // zero out all configuration entries
    for (int i = 0; i < NUM_PMP_CSRS; i++) {
        config->pmpxx[i] = 0;
    }

    // define 4 memory address ranges for Physical Memory Protection
    // 0 :: [0 .. user_payload]
    // 1 :: [user_payload .. .rodata]
    // 2 :: [.rodata .. stack_bottom]
    // 3 :: [stack_bottom .. paged_mem_end]
    config->num_available = NUM_PMP_CSRS - 5;
    set_pmpaddr(PMP_ADDR0_CSR + NUM_PMP_CSRS - 5, 0);
    set_pmpaddr(PMP_ADDR0_CSR + NUM_PMP_CSRS - 4, shift_right_addr(&user_payload, 2));
    set_pmpaddr(PMP_ADDR0_CSR + NUM_PMP_CSRS - 3, shift_right_addr(&rodata, 2));
    set_pmpaddr(PMP_ADDR0_CSR + NUM_PMP_CSRS - 2, shift_right_addr(&stack_bottom, 2));

    // finally, set all the heap pages to be read-writable by default. Make it
    // the last config entry in order to allow overrides by entries preceding it
    // (the PMP configs are applied in-order, the first that applies is the one
    // in effect).
    void *paged_mem_end = paged_mem_start + PAGE_SIZE * MAX_PAGES;
    void *end = shift_right_addr(paged_mem_end, 2);
    set_pmpaddr(PMP_ADDR0_CSR + NUM_PMP_CSRS - 1, end);

    // set the uppermost 4 PMP entries to TOR (Top Of the address Range)
    // addressing mode so that the associated address register forms the top of
    // the address range per entry, the type of PMP entry is encoded in 2 bits:
    // OFF = 0, TOR = 1, NA4 = 2, NAPOT = 3 and stored in 3:4 bits of every PMP
    // entry

    // set access flags for the PMP entries:
    // 0 :: [0 .. user_payload]                      000  M-mode kernel code, no access in U-mode
    // 1 :: [user_payload .. .rodata]                X0R  user code, executable, non-modifiable in U-mode
    // 2 :: [.rodata .. stack_bottom]                000  no access in U-mode
    // 3 :: [stack_bottom .. paged_mem_end]          0WR  user heap, modifiable, but no executable in U-mode
    // access (R)ead, (W)rite and e(X)ecute are 1 bit flags and stored in 0:2 bits of every PMP entry
    config->pmpxx[NUM_PMP_CSRS - 4] = PMP_TOR | 0;
    config->pmpxx[NUM_PMP_CSRS - 3] = PMP_TOR | PMP_X | PMP_R;
    config->pmpxx[NUM_PMP_CSRS - 2] = PMP_TOR | 0;
    config->pmpxx[NUM_PMP_CSRS - 1] = PMP_TOR | PMP_R | PMP_W;
#endif
}

// adjust_pmp_napot_addr takes a page address and converts it into a
// naturally-aligned power-of-two (NAPOT) address with a range size encoding.
// The address itself is right-shifted by two, just like elsewhere in PMP code,
// then the lowest bits encode the range size: a zero, followed by a number of
// ones representing the size. Size equals to 2**(number of ones + 3). For
// example, 0111111 for size=512.
void* adjust_pmp_napot_addr(void *addr) {
    regsize_t a = (regsize_t)shift_right_addr(addr, 2);
    a |= (PAGE_SIZE - 1) >> 3;  // set N-3 lowest bits to 1
    a &= ~(PAGE_SIZE >> 3);     // clear a bit above them
    return (void*)a;
}

void apply_pmp_config(pmp_config_t *config) {
#if HAS_PMP
#if __riscv_xlen == 32
    set_pmpcfg(PMP_CFG1_CSR, *(regsize_t*)(config->pmpxx + 4));
    set_pmpcfg(PMP_CFG3_CSR, *(regsize_t*)(config->pmpxx + 12));
#endif
    set_pmpcfg(PMP_CFG0_CSR, *(regsize_t*)config->pmpxx);
    set_pmpcfg(PMP_CFG2_CSR, *(regsize_t*)(config->pmpxx + XLEN/8));
#endif
}

void apply_ith_config(pmp_config_t *config, int i) {
    // TODO: do the math and set only one cfg register
    apply_pmp_config(config);
}
