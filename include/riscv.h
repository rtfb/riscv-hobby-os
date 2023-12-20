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

// pmpaddrXX CSRs are numbered 0x3b0, 0x3b1, 0x3b2, etc. In some cases it's more
// convenient to calculate the CSR number than to use its mnemonic, this
// constant is here to help. For example, use (PMP_ADDR0_CSR + 7) to get an
// equivalent of pmpaddr7.
#define PMP_ADDR0_CSR     0x3b0
#define PMP_ADDR1_CSR     (PMP_ADDR0_CSR + 1)
#define PMP_ADDR2_CSR     (PMP_ADDR0_CSR + 2)
#define PMP_ADDR3_CSR     (PMP_ADDR0_CSR + 3)
#define PMP_ADDR4_CSR     (PMP_ADDR0_CSR + 4)
#define PMP_ADDR5_CSR     (PMP_ADDR0_CSR + 5)
#define PMP_ADDR6_CSR     (PMP_ADDR0_CSR + 6)
#define PMP_ADDR7_CSR     (PMP_ADDR0_CSR + 7)
#define PMP_ADDR8_CSR     (PMP_ADDR0_CSR + 8)
#define PMP_ADDR9_CSR     (PMP_ADDR0_CSR + 9)
#define PMP_ADDR10_CSR    (PMP_ADDR0_CSR + 10)
#define PMP_ADDR11_CSR    (PMP_ADDR0_CSR + 11)
#define PMP_ADDR12_CSR    (PMP_ADDR0_CSR + 12)
#define PMP_ADDR13_CSR    (PMP_ADDR0_CSR + 13)
#define PMP_ADDR14_CSR    (PMP_ADDR0_CSR + 14)
#define PMP_ADDR15_CSR    (PMP_ADDR0_CSR + 15)

#define set_pmpaddr(csr_num, addr)  \
    __asm__ volatile (              \
        "csrw %0, %1"               \
        :                           \
        : "i"(csr_num), "r"(addr)   \
    )

#define PMP_CFG0_CSR     0x3a0
#define PMP_CFG2_CSR     (PMP_CFG0_CSR + 2)
#if __riscv_xlen == 32
#define PMP_CFG1_CSR     (PMP_CFG0_CSR + 1)
#define PMP_CFG3_CSR     (PMP_CFG0_CSR + 3)
#endif

#define set_pmpcfg(csr_num, valur)  \
    __asm__ volatile (              \
        "csrw %0, %1"               \
        :                           \
        : "i"(csr_num), "r"(valur)  \
    )

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

void set_user_mode();
void set_jump_address(void *func);
void set_scratch_csr(void* ptr);

// implemented in boot.S
void park_hart();

#endif // ifndef _RISCV_H_
