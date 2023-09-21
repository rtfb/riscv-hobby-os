#include "kernel.h"
#include "asm.h"

void set_status_interrupt_pending() {
    // set mstatus.MPIE (Machine Pending Interrupt Enable) bit to 1:
    unsigned int status_csr = get_status_csr();
#if TARGET_M_MODE
    status_csr |= (1 << MSTATUS_MPIE_BIT);
#else
    status_csr |= (1 << MSTATUS_SPIE_BIT);
#endif
    set_status_csr(status_csr);
}

void set_status_interrupt_enable_and_pending() {
    unsigned int status_csr = get_status_csr();
#if TARGET_M_MODE
    status_csr |= ((1 << MSTATUS_MIE_BIT) | (1 << MSTATUS_MPIE_BIT));
#else
    status_csr |= ((1 << MSTATUS_SIE_BIT) | (1 << MSTATUS_SPIE_BIT));
#endif
    set_status_csr(status_csr);
}

void clear_status_interrupt_enable() {
    unsigned int status_csr = get_status_csr();
#if TARGET_M_MODE
    status_csr &= ~(1 << MSTATUS_MIE_BIT);
#else
    status_csr &= ~(1 << MSTATUS_SIE_BIT);
#endif
    set_status_csr(status_csr);
}

void set_interrupt_enable_bits() {
    // set the mie.MTIE (Machine Timer Interrupt Enable) bit to 1:
#if TARGET_M_MODE
    unsigned int mie = (1 << MIE_MTIE_BIT) | (1 << MIE_MEIE_BIT);
#else
    unsigned int mie = (1 << MIE_STIE_BIT) | (1 << MIE_SEIE_BIT);
#endif
    set_ie_csr(mie);
}

unsigned int get_hartid() {
    return 0; // XXX: change that when SMP gets implemented
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

void set_status_csr(unsigned int value) {
    asm volatile (
        "csrw " REG_STATUS ", %0"
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
#ifdef TARGET_M_MODE
    status_csr &= MPP_MASK;    // zero out mode bits 11:12
    status_csr |= MPP_MODE_U;  // set them to user mode
#else
    status_csr &= SPP_MASK;    // zero out SPP mode bit 8
    status_csr |= SPP_MODE_U;  // set it to user mode
#endif
    set_status_csr(status_csr);
}

void set_scratch_csr(void* ptr) {
    asm volatile (
        "csrw " REG_SCRATCH ", %0"
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

void set_tvec_csr(void *ptr) {
    asm volatile (
        "csrw  " REG_TVEC ", %0;"   // set mtvec to the requested value
        :                           // no output
        : "r"(ptr)                  // input in ptr
    );
}
