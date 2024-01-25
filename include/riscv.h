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
#define MSTATUS_SUM_BIT  18    // Supervisor User Memory access: When SUM=0, S-mode memory accesses to pages that are accessible by U-mode
#define MSTATUS_MXR_BIT  19    // Make eXecutable Readable
#define MSTATUS_TVM_BIT  20    // Trap Virtual Memory

#define MIE_SSIE_BIT   1  // mie.SSIE (Supervisor-level Software Interrupt Enable) bit
#define MIE_MSIE_BIT   3  // mie.MSIE (Machine-level Software Interrupt Enable) bit
#define MIE_STIE_BIT   5  // mie.STIE (Supervisor Timer Interrupt Enable) bit
#define MIE_MTIE_BIT   7  // mie.MTIE (Machine Timer Interrupt Enable) bit
#define MIE_SEIE_BIT   9  // mie.SEIE (Supervisor External Interrupt Enable) bit
#define MIE_MEIE_BIT  11  // mie.MEIE (Machine External Interrupt Enable) bit

#define MIP_SSIP_BIT   1

#define SATP_MODE_SV39 ((regsize_t)(8) << 60)

#if __riscv_xlen == 64
#define MAKE_SATP(ptr) (PHYS_TO_PPN(ptr) | SATP_MODE_SV39)
#else
#define MAKE_SATP(ptr) 0
#endif

#define PTE_V          (1 << 0)
#define PTE_R          (1 << 1)
#define PTE_W          (1 << 2)
#define PTE_X          (1 << 3)
#define PTE_U          (1 << 4)

// XXX: remove PTE_W from kcode?
#define PERM_KCODE     (PTE_R | PTE_W | PTE_X)
#define PERM_KDATA     (PTE_R | PTE_W)
#define PERM_KRODATA   PTE_R
#define PERM_NONLEAF   PTE_V
#define PERM_UCODE     (PTE_U | PTE_R | PTE_W | PTE_X)
#define PERM_UDATA     (PTE_U | PTE_R | PTE_W)

// TODO: fix these:
#if HAS_S_MODE
#define UVA(a) (((regsize_t)a) & ~0x80000000)
#define UPA(a) (((regsize_t)a) | 0x80000000)
#else
#define UVA(a) (regsize_t)(a)
#define UPA(a) (regsize_t)(a)
#endif

#define PHYS_TO_PPN(paddr)   (((regsize_t)paddr) >> 12)
#define PHYS_TO_PTE(paddr)   (PHYS_TO_PPN(paddr) << 10)
#define PHYS_TO_PTE2(paddr)  (PPN1(paddr) << (10+9))
#define PPN_TO_PHYS(ppn)     (ppn << 12)
#define PTE_TO_PHYS(pte)     (void*)(PPN_TO_PHYS((regsize_t)pte >> 10))

#define IS_NONLEAF(pte)      ((pte & PTE_V) && !(pte & PTE_R) && !(pte & PTE_X))
#define IS_USER(pte)         ((pte & PTE_U) != 0)

#define PAGEROUND(p)   ((regsize_t)(p) & ~(PAGE_SIZE-1))
#define PAGEROUNDUP(p) (PAGEROUND(p) + PAGE_SIZE)

// 9 bits:
#define PPN0_MASK      0x1ff
#define PPN1_MASK      PPN0_MASK
// 26 bits:
#define PPN2_MASK      ((1 << 26) - 1)

#define PPN0_OFFS      12
#define PPN1_OFFS      (PPN0_OFFS + 9)
#define PPN2_OFFS      (PPN1_OFFS + 9)

#define PPN0(paddr)    (((regsize_t)paddr >> PPN0_OFFS) & PPN0_MASK)
#define PPN1(paddr)    ((paddr >> PPN1_OFFS) & PPN1_MASK)
#define PPN2(paddr)    ((((regsize_t)paddr) >> PPN2_OFFS) & PPN2_MASK)

// all VPNx fields are 9 bits:
#define VPNx_MASK      0x1ff

#define VPN(vaddr, lvl) ((vaddr >> (12+9*(lvl))) & VPNx_MASK)

#define SIP_SSIP       (1 << MIP_SSIP_BIT)

// dedicated M-Mode funcs:
unsigned int get_hartid();
void set_mscratch_csr(void* ptr);
void set_mtvec_csr(void* ptr);
void set_pmpaddr0(void* addr);
void set_pmpaddr1(void* addr);
void set_pmpaddr2(void* addr);
void set_pmpaddr3(void* addr);
void set_pmpcfg0(unsigned long value);
void set_mstatus_csr(unsigned int status);
void set_mepc_csr(void *ptr);
void set_mideleg_csr(regsize_t value);
void set_medeleg_csr(regsize_t value);
void set_mie_csr(regsize_t value);

// dedicated S-Mode funcs:
void set_sip_csr(regsize_t value);
void csr_sip_clear_flags(regsize_t flags);
regsize_t get_sip_csr();
void set_stvec_csr(void *ptr);
void set_sscratch_csr(void* ptr);
void set_satp(regsize_t value);

// ifdef-controlled M/S-Mode funcs:
unsigned int get_status_csr();
void set_status_csr(unsigned int status);
void* get_epc_csr();
void set_ie_csr(unsigned int value);
void set_user_mode();
void set_supervisor_mode();
void set_jump_address(void *func);

void set_status_interrupt_pending();
void set_status_interrupt_enable_and_pending();
void clear_status_interrupt_enable();
void set_interrupt_enable_bits();

// implemented in boot.S
void park_hart();

#endif // ifndef _RISCV_H_
