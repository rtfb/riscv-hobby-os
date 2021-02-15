#ifndef _KERNEL_H_
#define _KERNEL_H_

// 1.3 Privilege Levels, Table 1.1: RISC-V privilege levels.
#define MODE_U      0 << 11
#define MODE_S      1 << 11
#define MODE_M      3 << 11
#define MODE_MASK ~(3 << 11)

// These addresses are taken from the SiFive E31 core manual[1],
// Chapter 8: Core Local Interruptor (CLINT)
// [1] https://static.dev.sifive.com/E31-RISCVCoreIP.pdf
#define MTIME             0x200bff8
#define MTIMECMP_BASE     0x2004000

// Based on SIFIVE_CLINT_TIMEBASE_FREQ = 10000000 value from QEMU SiFive CLINT
// implementation:
// https://github.com/qemu/qemu/blob/master/include/hw/intc/sifive_clint.h
#define ONE_SECOND        10*1000*1000

void init_process_table();
void schedule_user_process();
void set_user_mode();
void jump_to_address(void *func);
unsigned int get_mstatus();
void set_mstatus(unsigned int mstatus);
void* get_mepc();
void kernel_timer_tick();
void set_timer();
void disable_interrupts();
void enable_interrupts();
void set_mie(unsigned int value);
void set_timer_after(uint64_t delta);
void kprints(char const *msg);
void kprintp(void* p);

extern void user_entry_point();
extern void user_entry_point2();

#endif // ifndef _KERNEL_H_
