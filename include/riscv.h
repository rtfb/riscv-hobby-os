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

#define MIE_SSIE_BIT   1  // mie.SSIE (Supervisor-level Software Interrupt Enable) bit
#define MIE_MSIE_BIT   3  // mie.MSIE (Machine-level Software Interrupt Enable) bit
#define MIE_STIE_BIT   5  // mie.STIE (Supervisor Timer Interrupt Enable) bit
#define MIE_MTIE_BIT   7  // mie.MTIE (Machine Timer Interrupt Enable) bit
#define MIE_SEIE_BIT   9  // mie.SEIE (Supervisor External Interrupt Enable) bit
#define MIE_MEIE_BIT  11  // mie.MEIE (Machine External Interrupt Enable) bit

#define MIP_SSIP_BIT   1

#define SIP_SSIP      (1 << MIP_SSIP_BIT)

#define SATP_MODE_SV39 (8UL << 60)

#if CONFIG_MMU
#define MAKE_SATP(ptr) (PHYS_TO_PPN(ptr) | SATP_MODE_SV39)
#define USR_VIRT(pa)   (((regsize_t)pa) & ~0xffe00000)
#else
#define MAKE_SATP(ptr) 0
#define USR_VIRT(a)    (regsize_t)(a)
#endif

#define PTE_V          (1 << 0)
#define PTE_R          (1 << 1)
#define PTE_W          (1 << 2)
#define PTE_X          (1 << 3)
#define PTE_U          (1 << 4)
#define PTE_G          (1 << 5)
#define PTE_A          (1 << 6)
#define PTE_D          (1 << 7)

#define PERM_KCODE     (PTE_R | PTE_X)
#define PERM_KDATA     (PTE_R | PTE_W)
#define PERM_KRODATA   PTE_R
#define PERM_NONLEAF   PTE_V
#define PERM_UCODE     (PTE_U | PTE_R | PTE_X)
#define PERM_UDATA     (PTE_U | PTE_R | PTE_W)
#define PERM_URODATA   (PTE_U | PTE_R)

#define PERM_MASK(pte) (pte & 0x3ff)

#define PHYS_TO_PPN(paddr)   (((regsize_t)paddr) >> 12)
#define PHYS_TO_PTE(paddr)   (PHYS_TO_PPN(paddr) << 10)
#define PPN_TO_PHYS(ppn)     (ppn << 12)
#define PTE_TO_PHYS(pte)     (void*)(PPN_TO_PHYS((regsize_t)(pte) >> 10))

#define HAS_V(pte)           ((pte & PTE_V) != 0)
#define HAS_R(pte)           ((pte & PTE_R) != 0)
#define HAS_W(pte)           ((pte & PTE_W) != 0)
#define HAS_X(pte)           ((pte & PTE_X) != 0)
#define HAS_U(pte)           ((pte & PTE_U) != 0)
#define HAS_G(pte)           ((pte & PTE_G) != 0)
#define HAS_A(pte)           ((pte & PTE_A) != 0)
#define HAS_D(pte)           ((pte & PTE_D) != 0)

#define IS_NONLEAF(pte)      (HAS_V(pte) && !HAS_R(pte) && !HAS_X(pte))
#define IS_USER(pte)         HAS_U(pte)
#define IS_VALID(pte)        (HAS_V(pte) || (!HAS_R(pte) && HAS_W(pte)))

// all VPNx fields are 9 bits:
#define VPNx_MASK      0x1ff

// extract level'th virtual page number of a given vaddr
#define VPN(vaddr, level) ((vaddr >> (12+9*(level))) & VPNx_MASK)

#define PAGE_OFFS(addr)   ((regsize_t)addr & (PAGE_SIZE - 1))

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
