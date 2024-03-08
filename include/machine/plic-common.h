#ifndef _PLIC_COMMON_H_
#define _PLIC_COMMON_H_

// definitions of PLIC constants for several machines: HiFive1-revB, Qemu
// sifive_u and Qemu sifive_e.

#define PLIC_NUM_INTR_SOURCES       53

#define PLIC_ENABLE_HART_OFFS       (0x80 * BOOT_HART_ID)
#define PLIC_THRESH_HART_OFFS       (0x1000 * BOOT_HART_ID)
#define PLIC_CLAIM_HART_OFFS        (0x1000 * BOOT_HART_ID)

#define PLIC_BASE                   0x0c000000
#define PLIC_PRIORITY  (PLIC_BASE + 0x00000000)
#define PLIC_PENDING   (PLIC_BASE + 0x00001000)
#define PLIC_ENABLE    (PLIC_BASE + 0x00002000 + PLIC_ENABLE_HART_OFFS)
#define PLIC_THRESHOLD (PLIC_BASE + 0x00200000 + PLIC_THRESH_HART_OFFS)
#define PLIC_CLAIM_RW  (PLIC_BASE + 0x00200004 + PLIC_CLAIM_HART_OFFS)

#define PLIC_MAX_PRIORITY  7

#endif // ifndef _PLIC_COMMON_H_
