#ifndef _PLIC_COMMON_H_
#define _PLIC_COMMON_H_

// definitions of PLIC constants for several machines: HiFive1-revB, Qemu
// sifive_u and Qemu sifive_e.

#define PLIC_NUM_INTR_SOURCES       53

#define PLIC_BASE                   0x0c000000
#define PLIC_PRIORITY  (PLIC_BASE + 0x00000000)
#define PLIC_PENDING   (PLIC_BASE + 0x00001000)
#define PLIC_ENABLE    (PLIC_BASE + 0x00002000)
#define PLIC_THRESHOLD (PLIC_BASE + 0x00200000)
#define PLIC_CLAIM_RW  (PLIC_BASE + 0x00200004)

#define PLIC_MAX_PRIORITY  7

#endif // ifndef _PLIC_COMMON_H_
