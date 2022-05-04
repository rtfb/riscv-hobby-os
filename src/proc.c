#include "proc.h"

process_t proc_table[MAX_PROCS];
int curr_proc = 0;
int num_procs = 0;
trap_frame_t trap_frame;

void init_process_table() {
    init_test_processes();
    // init curr_proc to -1, it will get incremented to 0 on the first
    // scheduler run. This will also help to identify the very first call to
    // kernel_timer_tick, which will happen from the kernel land. We want to
    // know that so we can discard the kernel code pc that we get on that first
    // call.
    curr_proc = -1;
}

void init_global_trap_frame() {
    set_mscratch(&trap_frame);
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
// (U-mode). MRET will change privilege to Machine Previous Privilege stored in
// mstatus CSR and jump to Machine Exception Program Counter specified by mepc
// CSR.
//
// schedule_user_process() is only called from kernel_timer_tick(), and MRET is
// called in interrupt_epilogue, after kernel_timer_tick() exits.
void schedule_user_process() {
    if (curr_proc < 0) {
        // compensate for curr_proc being initialized to -1 for the benefit of
        // identifying the very first kernel_timer_tick(), which gets a mepc
        // pointing to the kernel land, which we want to discard.
        curr_proc = 0;
    }
    if (num_procs == 0) {
        return;
    }
    process_t *last_proc = &proc_table[curr_proc];
    curr_proc++;
    if ((curr_proc >= MAX_PROCS) || (curr_proc >= num_procs)) {
        curr_proc = 0;
    }
    process_t *proc = &proc_table[curr_proc];
    if (last_proc->pid != proc->pid) {
        // the user process has changed: save the descending process's context
        // and load the ascending one's
        for (int i = 0; i < 32; i++) {
            last_proc->context.regs[i] = trap_frame.regs[i];
            trap_frame.regs[i] = proc->context.regs[i];
        }
    }
    set_jump_address(proc->pc);
    set_user_mode();
}
