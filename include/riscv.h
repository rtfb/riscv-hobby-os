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
// #define MTIME             0x200bff8
// #define MTIMECMP_BASE     0x2004000

#define CLINT_BASE                 0xe4000000
#define MTIME             (CLINT_BASE + 0xbff8)
#define MTIMECMP_BASE     (CLINT_BASE + 0x4000)
// #define STIMECMP          (0xe400D000)

// #define TIMER1_BASE   0x30009000
// #define MTIME             (TIMER1_BASE + 0xbff8)
// #define MTIMECMP_BASE     (TIMER1_BASE + 0xd000)

#define OX64_TCCR_REG     0x2000a500    // timer control register
#define OX64_TMR2_0       0x2000a510    // timer2 comparator 0 value
#define OX64_TCR2_REG     0x2000a52c    // timer2 counter
#define OX64_TIER2_REG    0x2000a544    // timer2 interrupt enable register
#define OX64_TICR2_REG    0x2000a578    // timer2 interrupt clear register
#define OX64_TCER_REG     0x2000a584    // timer count enable register
#define OX64_TCMR_REG     0x2000a588    // timer count mode register


#define TRAP_DIRECT   0x00
#define TRAP_VECTORED 0x01

#define MSTATUS_SIE_BIT   1
#define MSTATUS_SPIE_BIT  5
#define MSTATUS_MPIE_BIT  7

#define MIE_STIE_BIT   5  // mie.STIE (Supervisor Timer Interrupt Enable) bit
#define MIE_MTIE_BIT   7  // mie.MTIE (Machine Timer Interrupt Enable) bit
#define MIE_SEIE_BIT   9  // mie.SEIE (Supervisor External Interrupt Enable) bit
#define MIE_MEIE_BIT  11  // mie.MEIE (Machine External Interrupt Enable) bit

unsigned int get_mhartid();
uint64_t get_mstatus();
void set_mstatus(uint64_t mstatus);
void* get_mepc();
void set_mie(uint64_t value);
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

void timer_init();
void set_timer_after(uint64_t delta);
uint64_t time_get_now();

#endif // ifndef _RISCV_H_
