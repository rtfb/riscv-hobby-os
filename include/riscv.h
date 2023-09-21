#ifndef _RISCV_H_
#define _RISCV_H_

#include "sys.h"

// An arbitrary limit, as of now:
#define MAX_HARTS 4

// 1.3 Privilege Levels, Table 1.1: RISC-V privilege levels.
#define MPP_MODE_U      0 << 11
#define MPP_MODE_S      1 << 11
#define MPP_MODE_M      3 << 11
#define MPP_MASK        ~(3 << 11)
#define SPP_MASK        ~(1 << 8)
#define SPP_MODE_U      0 << 8

#define TRAP_DIRECT   0x00
#define TRAP_VECTORED 0x01

#define MSTATUS_SIE_BIT   1
#define MSTATUS_MIE_BIT   3
#define MSTATUS_SPIE_BIT  5
#define MSTATUS_MPIE_BIT  7

#define MIE_STIE_BIT   5  // mie.STIE (Supervisor Timer Interrupt Enable) bit
#define MIE_MTIE_BIT   7  // mie.MTIE (Machine Timer Interrupt Enable) bit
#define MIE_SEIE_BIT   9  // mie.SEIE (Supervisor External Interrupt Enable) bit
#define MIE_MEIE_BIT  11  // mie.MEIE (Machine External Interrupt Enable) bit

void set_status_interrupt_pending();
void set_status_interrupt_enable_and_pending();
void clear_status_interrupt_enable();
void set_interrupt_enable_bits();

unsigned int get_hartid();
unsigned int get_status_csr();
void set_status_csr(unsigned int status);
void* get_epc_csr();
void set_ie_csr(unsigned int value);
void set_tvec_csr(void *ptr);

void set_pmpaddr0(void* addr);
void set_pmpaddr1(void* addr);
void set_pmpaddr2(void* addr);
void set_pmpaddr3(void* addr);
void set_pmpcfg0(unsigned long value);

void set_user_mode();
void set_jump_address(void *func);
void set_scratch_csr(void* ptr);

// implemented in boot.S
void park_hart();

#endif // ifndef _RISCV_H_
