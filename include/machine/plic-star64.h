#ifndef _PLIC_COMMON_H_
#define _PLIC_COMMON_H_

// definitions of PLIC constants for several machines: HiFive1-revB, Qemu
// sifive_u and Qemu sifive_e.

// below this line it's a verbatim copy of plic-common.h
// TODO: unify

#if HAS_S_MODE
#define S_MODE_HART_MULTIPLIER_OFFS   1
#else
#define S_MODE_HART_MULTIPLIER_OFFS   0
#endif

#define PLIC_ENABLE_HART_OFFS       (0x80 * (BOOT_HART_ID + S_MODE_HART_MULTIPLIER_OFFS))
#define PLIC_THRESH_HART_OFFS       (0x1000 * (BOOT_HART_ID + S_MODE_HART_MULTIPLIER_OFFS))
#define PLIC_CLAIM_HART_OFFS        (0x1000 * (BOOT_HART_ID + S_MODE_HART_MULTIPLIER_OFFS))

#define PLIC_PRIORITY  (PLIC_BASE + 0x00000000)
#define PLIC_PENDING   (PLIC_BASE + 0x00001000)
#define PLIC_ENABLE    (PLIC_BASE + 0x00002000 + PLIC_ENABLE_HART_OFFS)
#define PLIC_THRESHOLD (PLIC_BASE + 0x00200000 + PLIC_THRESH_HART_OFFS)
#define PLIC_CLAIM_RW  (PLIC_BASE + 0x00200004 + PLIC_CLAIM_HART_OFFS)

#endif // ifndef _PLIC_COMMON_H_
