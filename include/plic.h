#ifndef _PLIC_H_
#define _PLIC_H_

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
