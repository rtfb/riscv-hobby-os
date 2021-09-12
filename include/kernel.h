#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "riscv.h"
#include "pmp.h"

// Based on SIFIVE_CLINT_TIMEBASE_FREQ = 10000000 value from QEMU SiFive CLINT
// implementation:
// https://github.com/qemu/qemu/blob/master/include/hw/intc/sifive_clint.h
#define ONE_SECOND        10*1000*1000

#define KERNEL_SCHEDULER_TICK_TIME (ONE_SECOND)

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
void kprints(char const *msg);
void kprintp(void* p);

extern void user_entry_point();
extern void user_entry_point2();

#endif // ifndef _KERNEL_H_
