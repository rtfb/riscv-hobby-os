#include "sys.h"
#include "kernel.h"

typedef struct process_s {
    int pid;
    void *pc;
} process_t;

process_t proc_table[2];
int curr_proc = 0;

void kmain() {
    kprints("kmain\n");
    void *p = (void*)0xf10a;
    kprintp(p);
    proc_table[0].pid = 1;
    proc_table[0].pc = user_entry_point;
    proc_table[1].pid = 2;
    proc_table[1].pc = user_entry_point2;
    kernel_timer_tick();
}

// kernel_timer_tick will be called from timer to give kernel time to do its
// housekeeping as well as run the scheduler to pick the next user process to
// run.
void kernel_timer_tick() {
    kprints("K");
    void* userland_pc = (void*)get_mepc();
    if (userland_pc) {
        proc_table[curr_proc].pc = userland_pc;
    }
    disable_interrupts();
    set_timer_after(ONE_SECOND);
    enable_interrupts();
    schedule_user_process();
}

void schedule_user_process() {
    set_user_mode();
    curr_proc++;
    if (curr_proc > 1) {
        curr_proc = 0;
    }
    jump_to_address(proc_table[curr_proc].pc);
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
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void set_mstatus(unsigned int value) {
    asm volatile (
        "csrw mstatus, %0"
        :            // no output
        : "r"(value) // input in value
    );
}

void* get_mepc() {
    register void* a0 asm ("a0");
    asm volatile (
        "csrr a0, mepc"
        : "=r"(a0)   // output in a0
    );
    return a0;
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
void jump_to_address(void *func) {
    asm volatile (
        "csrw   mepc, %0;"   // set mepc to userland function
        "mret"               // return from timer interrupt handler to mepc
        :            // no output
        : "r"(func)  // input in func
    );
}

unsigned int get_hart_id() {
    register int a0 asm ("a0");
    asm volatile (
        "csrr a0, mhartid"
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void set_timer_after(uint64_t delta) {
    unsigned int hart_id = get_hart_id();
    uint64_t *mtime = (uint64_t*)MTIME;
    uint64_t *mtimecmp = (uint64_t*)(MTIMECMP_BASE) + 8*hart_id;
    uint64_t now = *mtime;
    *mtimecmp = now + delta;
}

void set_mie(unsigned int value) {
    asm volatile (
        "csrs   mie, %0;"   // set mie to the requested value
        :            // no output
        : "r"(value) // input in value
    );
}

void disable_interrupts() {
    set_mie(0);
}

void enable_interrupts() {
    // set mstatus.MIE (Machine Interrupt Enable) bit to 1:
    unsigned int mstatus = get_mstatus();
    mstatus |= 1 << 3;
    set_mstatus(mstatus);

    // set the mie.MTIE (Machine Timer Interrupt Enable) bit to 1:
    set_mie(1 << 7);
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
