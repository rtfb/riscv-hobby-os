#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "riscv.h"
#include "pmp.h"

#define KERNEL_SCHEDULER_TICK_TIME (ONE_SECOND)

void kinit(uintptr_t fdt_header_addr);
void init_process_table();
void init_trap_vector();
void schedule_user_process();
void set_user_mode();
void set_jump_address(void *func);
void kernel_timer_tick();
void set_timer();
void disable_interrupts();
void enable_interrupts();
void set_timer_after(uint64_t delta);

// implemented in boot.s
void kprints(char const *msg, ...);

// implemented in kernel.c
void kprintp(void* p);
void kprintul(unsigned long i);

extern void user_entry_point();
extern void user_entry_point2();

#endif // ifndef _KERNEL_H_
