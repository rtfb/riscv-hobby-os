#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "kprintf.h"

void kinit(regsize_t hartid, uintptr_t fdt_header_addr);
void init_trap_vector(regsize_t hartid);
void kernel_timer_tick(regsize_t sp);
void set_timer();
void disable_interrupts();
void enable_interrupts();
void test_kprintf();

// defined in boot.S
extern void* trap_vector;

// defined in kernel.c
extern int user_stack_size;

// panic prints "panic: <message>" and halts the CPU.
void panic(char const *message);

#endif // ifndef _KERNEL_H_
