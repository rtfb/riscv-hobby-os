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

#ifdef TARGET_M_MODE
    // define 4 memory address ranges for Physical Memory Protection
    // 0 :: [0 .. user_payload]
    // 1 :: [user_payload .. .rodata]
    // 2 :: [.rodata .. stack_bottom]
    // 3 :: [stack_bottom .. paged_mem_end]
    set_pmpaddr0(&user_payload);
    set_pmpaddr1(&rodata);
    set_pmpaddr2(&stack_bottom);
    set_pmpaddr3(paged_mem_end);

    // set 4 PMP entries to TOR (Top Of the address Range) addressing mode so that the associated
    // address register forms the top of the address range per entry,
    // the type of PMP entry is encoded in 2 bits: OFF = 0, TOR = 1, NA4 = 2, NAPOT = 3
    // and stored in 3:4 bits of every PMP entry
    unsigned long mode = PMP_TOR * (PMP_0 | PMP_1 | PMP_2 | PMP_3);

    // set access flags for 4 PMP entries:
    // 0 :: [0 .. user_payload]                      000  M-mode kernel code, no access in U-mode
    // 1 :: [user_payload .. .rodata]                X0R  user code, executable, non-modifiable in U-mode
    // 2 :: [.rodata .. stack_bottom]                000  no access in U-mode
    // 3 :: [stack_bottom .. paged_mem_end]          0WR  user stack, modifiable, but no executable in U-mode
    // access (R)ead, (W)rite and e(X)ecute are 1 bit flags and stored in 0:2 bits of every PMP entry
    unsigned long access_flags =   ((PMP_X | PMP_R) * PMP_1)
                                 | ((PMP_W | PMP_R) * PMP_3);
    set_pmpcfg0(mode | access_flags);
#endif

    return paged_mem_end;
}
