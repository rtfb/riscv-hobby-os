#ifndef _RISCV_H_
#define _RISCV_H_

#include "sys.h"

// An arbitrary limit, as of now:
#define MAX_HARTS 4

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

#define TRAP_DIRECT   0x00
#define TRAP_VECTORED 0x01

unsigned int get_mhartid();
unsigned int get_mstatus();
void set_mstatus(unsigned int mstatus);
void* get_mepc();
void set_mie(unsigned int value);
void set_mtvec(void *ptr);

void set_pmpaddr0(void* addr);
void set_pmpaddr1(void* addr);
void set_pmpaddr2(void* addr);
void set_pmpaddr3(void* addr);
void set_pmpcfg0(unsigned long value);

void set_user_mode();
void set_jump_address(void *func);
void set_mscratch(void* ptr);

// implemented in boot.s
void park_hart();

void set_timer_after(uint64_t delta);
uint64_t time_get_now();

#endif // ifndef _RISCV_H_
