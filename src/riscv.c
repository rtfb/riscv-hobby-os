#include "asm.h"
#include "riscv.h"
#include "timer.h"

int unsleep_scheduler = 0;

void set_status_interrupt_pending() {
    // set mstatus.MPIE (Machine Pending Interrupt Enable) bit to 1:
    unsigned int status_csr = get_status_csr();
#if !HAS_S_MODE
    status_csr |= (1 << MSTATUS_MPIE_BIT);
#else
    status_csr |= (1 << MSTATUS_SPIE_BIT);
#endif
    set_status_csr(status_csr);
}

void set_status_interrupt_enable_and_pending() {
    unsigned int status_csr = get_status_csr();
#if !HAS_S_MODE
    status_csr |= ((1 << MSTATUS_MIE_BIT) | (1 << MSTATUS_MPIE_BIT));
#else
    status_csr |= ((1 << MSTATUS_SIE_BIT) | (1 << MSTATUS_SPIE_BIT));
#endif
    set_status_csr(status_csr);
}

void clear_status_interrupt_enable() {
    unsigned int status_csr = get_status_csr();
#if !HAS_S_MODE
    status_csr &= ~(1 << MSTATUS_MIE_BIT);
#else
    status_csr &= ~(1 << MSTATUS_SIE_BIT);
#endif
    set_status_csr(status_csr);
}

void set_interrupt_enable_bits() {
    // set the mie.MTIE (Machine Timer Interrupt Enable) bit to 1:
#if !HAS_S_MODE
    unsigned int mie = (1 << MIE_MTIE_BIT) | (1 << MIE_MEIE_BIT);
#else
    unsigned int mie = (1 << MIE_STIE_BIT) | (1 << MIE_SEIE_BIT) | (1 << MIE_SSIE_BIT);
#endif
    set_ie_csr(mie);
}

unsigned int get_tp() {
    register unsigned int a0 asm ("a0");
    asm volatile (
        "mv a0, tp"
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void* shift_right_addr(void* addr, int bits) {
    unsigned long iaddr = (unsigned long)addr;
    return (void*)(iaddr >> bits);
}

void set_pmpaddr0(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr0, %0;"  // set pmpaddr0 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpaddr1(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr1, %0;"  // set pmpaddr1 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpaddr2(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr2, %0;"  // set pmpaddr2 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpaddr3(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr3, %0;"  // set pmpaddr3 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpcfg0(unsigned long value) {
    asm volatile (
        "csrw   pmpcfg0, %0;"   // set pmpcfg0 to the requested value
        :                       // no output
        : "r"(value)            // input in value
    );
}

unsigned int get_status_csr() {
    register unsigned int a0 asm ("a0");
    asm volatile (
        "csrr a0, " REG_STATUS
        : "=r"(a0)   // output in a0
    );
    return a0;
}

unsigned int get_mstatus_csr() {
    register unsigned int a0 asm ("a0");
    asm volatile (
        "csrr a0, mstatus"
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void set_status_csr(unsigned int value) {
    asm volatile (
        "csrw " REG_STATUS ", %0"
        :            // no output
        : "r"(value) // input in value
    );
}

void set_mstatus_csr(unsigned int value) {
    asm volatile (
        "csrw mstatus, %0"
        :            // no output
        : "r"(value) // input in value
    );
}

void* get_epc_csr() {
    register void* a0 asm ("a0");
    asm volatile (
        "csrr a0, " REG_EPC
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void set_jump_address(void *func) {
    asm volatile (
        "csrw " REG_EPC ", %0;"  // set mepc to userland function
        :                        // no output
        : "r"(func)              // input in func
    );
}

// Privilege levels are encoded in 2 bits: User = 0b00, Supervisor = 0b01,
// Machine = 0b11 and stored in 11:12 bits of mstatus CSR (called mstatus.mpp)
void set_user_mode() {
    unsigned int status_csr = get_status_csr();
#if !HAS_S_MODE
    status_csr &= MPP_MASK;    // zero out mode bits 11:12
    status_csr |= MPP_MODE_U;  // set them to user mode
#else
    status_csr &= SPP_MASK;    // zero out SPP mode bit 8
    status_csr |= SPP_MODE_U;  // set it to user mode
#endif
    set_status_csr(status_csr);
}

// set_supervisor_mode switches current hart to S-Mode. It also prepares
// several relevant CSRs for operation in S-Mode:
//   * sets mideleg/medeleg for trap delegation
//   * sets pending mode to S
//   * enables S-Mode interrupts and pending M-Mode interrupts
//   * enables timer, external and software interrupts in mie register for both
//     S and M modes
//
// TODO: make sure we've addressed things described in section
// 3.1.18: Machine Environment Configuration Registers (menvcfg and menvcfgh)
void set_supervisor_mode() {
    set_mideleg_csr((1 << MIE_STIE_BIT) | (1 << MIE_SEIE_BIT) | (1 << MIE_SSIE_BIT));
    set_medeleg_csr(~0);

    unsigned int mstatus = get_mstatus_csr();
    mstatus &= MPP_MASK;    // zero out mode bits 11:12
    mstatus |= MPP_MODE_S;  // set them to Supervisor mode
    mstatus |= (1 << MSTATUS_SIE_BIT) | (1 << MSTATUS_MPIE_BIT);
    set_mstatus_csr(mstatus);

    regsize_t mie = (1 << MIE_STIE_BIT) | (1 << MIE_SEIE_BIT) | (1 << MIE_SSIE_BIT)
        | (1 << MIE_MTIE_BIT) | (1 << MIE_MEIE_BIT) | (1 << MIE_MSIE_BIT);
    set_mie_csr(mie);

    // Call mret to switch to S-Mode. But there's a catch: mret not only
    // switches the mode, it also "returns" to wherever mepc points to. So set
    // mepc to point to the next instruction after mret, which effectively will
    // jump to this function's epilogue and the code flow will continue
    // undisturbed, but now in S-Mode.
    asm volatile (
        "                     \n\
        auipc  t0, 0          \n\
        addi   t0, t0, 16     \n\
        csrw   mepc, t0       \n\
        mret"
        ::
    );
}

void set_sscratch_csr(void* ptr) {
    asm volatile (
        "csrw sscratch, %0"
        :            // no output
        : "r"(ptr)   // input in value
    );
}

void set_mscratch_csr(void* ptr) {
    asm volatile (
        "csrw mscratch, %0"
        :            // no output
        : "r"(ptr)   // input in value
    );
}

void set_mtvec_csr(void* ptr) {
    asm volatile (
        "csrw mtvec, %0"
        :            // no output
        : "r"(ptr)   // input in value
    );
}

void set_mideleg_csr(regsize_t value) {
    asm volatile (
        "csrw mideleg, %0"
        :            // no output
        : "r"(value) // input in value
    );
}

void set_medeleg_csr(regsize_t value) {
    asm volatile (
        "csrw medeleg, %0"
        :            // no output
        : "r"(value) // input in value
    );
}

void set_mie_csr(regsize_t value) {
    asm volatile (
        "csrw mie, %0"
        :            // no output
        : "r"(value) // input in value
    );
}

void set_stvec_csr(void *ptr) {
    asm volatile (
        "csrw  stvec, %0;"   // set stvec to the requested value
        :                    // no output
        : "r"(ptr)           // input in ptr
    );
}

void set_mepc_csr(void* ptr) {
    asm volatile (
        "csrw mepc, %0"
        :            // no output
        : "r"(ptr)   // input in value
    );
}

void set_ie_csr(unsigned int value) {
    asm volatile (
        "csrs " REG_IE ", %0;"  // set mie to the requested value
        :                       // no output
        : "r"(value)            // input in value
    );
}

void csr_sip_clear_flags(regsize_t flags) {
    asm volatile (
        "csrc sip, %0;"  // clear requested bits of sip
        :                // no output
        : "r"(flags)     // input in flags
    );
}

// csr_sip_set_flags sets the requested bits of sip to be set to 1. Effectively,
// it does an atomic "sip |= flags".
void csr_sip_set_flags(regsize_t flags) {
    asm volatile (
        "csrs sip, %0;"  // set requested bits of sip
        :                // no output
        : "r"(flags)     // input in flags
    );
}

// hard_park_hart is an infinite loop, it will stop the calling hart forever.
// The wfi (wait for interrupt) instruction is just a hint to the core that it
// may downclock itself or otherwise save energy, not that we expect any
// interrupts to happen on it.
//
// It's meant to be called in cases like panic(), or to suspend an unused hart.
void hard_park_hart() {
    while (1) {
        asm volatile ("wfi");
    }
}

// soft_park_hart suspends a hart, but with a way out of it. If some conditions
// set unsleep_scheduler to true, it will immediately cause a timer interrupt,
// which in turn will let the scheduler do its thing.
void soft_park_hart() {
    while (1) {
        asm volatile ("wfi");
        if (unsleep_scheduler) {
            unsleep_scheduler = 0;
            cause_timer_interrupt_now();
        }
    }
}
