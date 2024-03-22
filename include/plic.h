#ifndef _PLIC_H_
#define _PLIC_H_

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

// plic_init prepares PLIC: disables all interrupts before devices enable the
// ones that they really care about.
void plic_init();

// plic_enable_intr enables a specified interrupt.
void plic_enable_intr(int intr_no);

// plic_set_intr_priority sets a priority on a given interrupt. 7 is the
// highest priority, while a zero priority effectively disables an interrupt.
void plic_set_intr_priority(int intr_no, int priority);

// plic_set_threshold sets a global threshold for PLIC interrupts. Only the
// interrupts with priority>threshold will be triggered.
void plic_set_threshold(int threshold);

// plic_dispatch_interrupts does the actual job of handling a PLIC interrupt
// and calling the respective subsystems.
void plic_dispatch_interrupts();

#endif // ifndef _PLIC_H_
