#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "sys.h"

void kinit(regsize_t hartid, uintptr_t fdt_header_addr);
void init_trap_vector();
void kernel_timer_tick(regsize_t sp);
void set_timer();
void disable_interrupts();
void enable_interrupts();

// defined in boot.S
extern void* trap_vector;

// defined in kernel.c
extern int user_stack_size;

// implemented in kprintf.S
// NOTE: gcc has a neat attribute, like this:
//     void kprintf(char const *msg, ...) __attribute__ ((format (printf, 1, 2)));
// with it, we'd get compile-time checks for format string compatibility with
// the args. However, that check is obviously very strict and demands me to use,
// e.g. %lx instead of %x on rv32. I don't want to bother with that at this
// point, so will just leave this note here as a reminder and we can implement
// it later.
// (docs: https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Function-Attributes.html)
extern void kprintf(char const *msg, ...);

// implement in kprintf.c
extern int32_t kprintfvec(char const* fmt, regsize_t* args);

// panic prints "panic: <message>" and halts the CPU.
void panic(char const *message);

#endif // ifndef _KERNEL_H_
