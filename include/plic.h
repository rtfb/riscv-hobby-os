#ifndef _PLIC_H_
#define _PLIC_H_

#define PLIC_NUM_INTR_SOURCES       53

#define PLIC_BASE                   0x0c000000
#define PLIC_PRIORITY  (PLIC_BASE + 0x00000000)
#define PLIC_PENDING   (PLIC_BASE + 0x00001000)
#define PLIC_ENABLE    (PLIC_BASE + 0x00002000)
#define PLIC_THRESHOLD (PLIC_BASE + 0x00200000)
#define PLIC_CLAIM_RW  (PLIC_BASE + 0x00200004)

#define PLIC_MAX_PRIORITY  7

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
