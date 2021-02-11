#include "kernel.h"

void kmain() {
    kprints("kmain\n");
    void *p = (void*)0xf10a;
    kprintp(p);
    schedule_user_process();
}

void schedule_user_process() {
    set_user_mode();
    jump_to_func(user_entry_point);
}

// Privilege levels are encoded in 2 bits: User = 0b00, Supervisor = 0b01,
// Machine = 0b11 and stored in 11:12 bits of mstatus CSR (called mstatus.mpp)
void set_user_mode() {
    unsigned int mstatus = get_mstatus();
    mstatus &= MODE_MASK;   // zero out mode bits 11:12
    mstatus |= MODE_U;      // set them to user mode
    set_mstatus(mstatus);
}

unsigned int get_mstatus() {
    register unsigned int a0 asm ("a0");
    asm volatile (
        "csrr a0, mstatus"
        : "=r"(a0)      // output in a0
    );
    return a0;
}

void set_mstatus(unsigned int mstatus) {
    register unsigned int a0 asm ("a0");
    asm volatile (
        "csrw mstatus, a0"
        :               // no output
        : "r"(a0)       // input in a0
    );
}

// 3.1.7 Privilege and Global Interrupt-Enable Stack in mstatus register
// > The MRET, SRET, or URET instructions are used to return from traps in
// > M-mode, S-mode, or U-mode respectively. When executing an xRET instruction,
// > supposing x PP holds the value y, x IE is set to xPIE; the privilege mode
// > is changed to > y; xPIE is set to 1; and xPP is set to U (or M if user-mode
// > is not supported).
//
// 3.2.2 Trap-Return Instructions
// > An xRET instruction can be executed in privilege mode x or higher,
// > where executing a lower-privilege xRET instruction will pop
// > the relevant lower-privilege interrupt enable and privilege mode stack.
// > In addition to manipulating the privilege stack as described in Section
// > 3.1.7, xRET sets the pc to the value stored in the x epc register.
//
// Use MRET instruction to switch privilege level from Machine (M-mode) to User
// (U-mode) MRET will change privilege to Machine Previous Privilege stored in
// mstatus CSR and jump to Machine Exception Program Counter specified by mepc
// CSR.
void jump_to_func(void *func) {
    asm volatile (
        "csrw   mepc, %0;"   // set mepc to userland function
        "mret;"              // mret will jump to what mepc points to
        :               // no output
        : "r"(func)     // input in func
    );
}

void kprintp(void* p) {
    static char hex_table[] = "0123456789abcdef";
    char buf[256];
    int i = 15;
    unsigned long pp = (unsigned long)p;
    while (i >= 0) {
        char lowest_4_bits = pp & 0xf;
        buf[i] = hex_table[lowest_4_bits];
        i--;
        pp >>= 4;
    }
    buf[16] = '\n';
    buf[17] = 0;
    kprints(buf);
}
