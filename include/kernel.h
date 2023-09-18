#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "riscv.h"
#include "pmp.h"
#include "proc.h"

#define KERNEL_SCHEDULER_TICK_TIME (ONE_SECOND)

void kinit(regsize_t hartid, uintptr_t fdt_header_addr);
void init_trap_vector();
void kernel_timer_tick();
void set_timer();
void disable_interrupts();
void enable_interrupts();
void set_timer_after(uint64_t delta);

// implemented in boot.s
// NOTE: gcc has a neat attribute, like this:
//     void kprintf(char const *msg, ...) __attribute__ ((format (printf, 1, 2)));
// with it, we'd get compile-time checks for format string compatibility with
// the args. However, that check is obviously very strict and demands me to use,
// e.g. %lx instead of %x on rv32. I don't want to bother with that at this
// point, so will just leave this note here as a reminder and we can implement
// it later.
// (docs: https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Function-Attributes.html)
extern void kprintf(char const *msg, ...);

extern int32_t kprintfvec(char const* fmt, regsize_t* args);

#endif // ifndef _KERNEL_H_
