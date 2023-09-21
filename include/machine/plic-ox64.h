#ifndef _PLIC_OX64_H_
#define _PLIC_OX64_H_

#define PLIC_NUM_INTR_SOURCES       21

#define PLIC_BASE                   0xe0000000
#define PLIC_PRIORITY  (PLIC_BASE + 0x00000000)
#define PLIC_PENDING   (PLIC_BASE + 0x00001000)

// S-mode for core0:
#define PLIC_ENABLE    (PLIC_BASE + 0x00002080)
#define PLIC_THRESHOLD (PLIC_BASE + 0x00201000)
#define PLIC_CLAIM_RW  (PLIC_BASE + 0x00201004)

#define PLIC_MAX_PRIORITY           7

#endif // ifndef _PLIC_OX64_H_
