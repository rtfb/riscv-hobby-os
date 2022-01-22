#include "proc.h"
#include "pagealloc.h"

proc_table_t proc_table;
trap_frame_t trap_frame;

void init_process_table() {
    // init curr_proc to -1, it will get incremented to 0 on the first
    // scheduler run. This will also help to identify the very first call to
    // kernel_timer_tick, which will happen from the kernel land. We want to
    // know that so we can discard the kernel code pc that we get on that first
    // call.
    proc_table.curr_proc = -1;
    proc_table.pid_counter = 0;
    for (int i = 0; i < MAX_PROCS; i++) {
        proc_table.procs[i].state = PROC_STATE_AVAILABLE;
    }
    init_test_processes();
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
    acquire(&proc_table.lock);
    int curr_proc = proc_table.curr_proc;
    process_t *last_proc = 0;
    if (curr_proc < 0) {
        // compensate for curr_proc being initialized to -1 for the benefit of
        // identifying the very first kernel_timer_tick(), which gets a mepc
        // pointing to the kernel land, which we want to discard.
        curr_proc = 0;
    } else {
        last_proc = &proc_table.procs[curr_proc];
        if (last_proc->state == PROC_STATE_AVAILABLE) {
            // schedule_user_process may have been called from proc_exit, which
            // kills the process in curr_proc slot, so if that's the case,
            // pretend there wasn't any last_proc:
            last_proc = 0;
        }
    }
    if (proc_table.num_procs == 0) {
        release(&proc_table.lock);
        return;
    }

    process_t *proc = find_ready_proc(curr_proc);
    acquire(&proc->lock);
    proc->state = PROC_STATE_RUNNING;

    if (last_proc == 0) {
        copy_context(&trap_frame, &proc->context);
    } else if (last_proc->pid != proc->pid) {
        // the user process has changed: save the descending process's context
        // and load the ascending one's
        acquire(&last_proc->lock);
        copy_context(&last_proc->context, &trap_frame);
        last_proc->state = PROC_STATE_READY;
        release(&last_proc->lock);
        copy_context(&trap_frame, &proc->context);
    }
    set_jump_address(proc->pc);
    release(&proc->lock);
    release(&proc_table.lock);
    set_user_mode();
}

process_t* find_ready_proc(int curr_proc) {
    int orig_curr_proc = curr_proc;
    do {
        curr_proc++;
        if (curr_proc >= MAX_PROCS) {
            curr_proc = 0;
        }
        if (proc_table.procs[curr_proc].state == PROC_STATE_READY) {
            break;
        }
    } while (curr_proc != orig_curr_proc);
    proc_table.curr_proc = curr_proc;
    return &proc_table.procs[curr_proc];
}

uint32_t proc_fork() {
    // allocate stack. Fail early if we're out of memory:
    void* sp = allocate_page();
    if (!sp) {
        // TODO: set errno
        return -1;
    }

    process_t* parent = current_proc();
    if (parent == 0) {
        // this should never happen, some process called us, right?
        // TODO: panic
        return -1;
    }
    acquire(&parent->lock);
    parent->pc = get_mepc();
    parent->pc += 4; // TODO: shouldn't this be +=2 on a compressed ISA? But it
                     // works fine, though... Need to step this through with a
                     // debugger.
    copy_context(&parent->context, &trap_frame);

    process_t* child = alloc_process();
    if (!child) {
        release(&parent->lock);
        return -1;
    }
    child->pid = alloc_pid();
    child->parent_pid = parent->pid;
    child->pc = parent->pc;
    child->stack_page = sp;
    copy_page(child->stack_page, parent->stack_page);
    copy_context(&child->context, &parent->context);

    // overwrite the sp with the same offset as parent->sp, but within the child stack:
    // regsize_t offset = (regsize_t)(parent->stack_page - parent->context.regs[REG_SP]);
    regsize_t offset = parent->context.regs[REG_SP] - (regsize_t)parent->stack_page;
    // regsize_t offset = trap_frame.regs[REG_SP] - (regsize_t)parent->stack_page;
    child->context.regs[REG_SP] = (regsize_t)(sp + offset);
    offset = parent->context.regs[REG_FP] - (regsize_t)parent->stack_page;
    child->context.regs[REG_FP] = (regsize_t)(sp + offset);
    // child's return value should be a 0 pid:
    child->context.regs[REG_A0] = 0;
    release(&parent->lock);
    release(&child->lock);
    trap_frame.regs[REG_A0] = child->pid;
    return child->pid;
}

// Let's start with a trivial implementation: a forever increasing counter.
uint32_t alloc_pid() {
    acquire(&proc_table.lock);
    uint32_t pid = proc_table.pid_counter;
    proc_table.pid_counter++;
    release(&proc_table.lock);
    return pid;
}

process_t* alloc_process() {
    acquire(&proc_table.lock);
    for (int i = 0; i < MAX_PROCS; i++) {
        if (i == proc_table.curr_proc) {
            continue;
        }
        if (proc_table.procs[i].state == PROC_STATE_AVAILABLE) {
            process_t* proc = &proc_table.procs[i];
            acquire(&proc->lock);
            proc->state = PROC_STATE_READY;
            proc_table.num_procs++;
            release(&proc_table.lock);
            return proc;
        }
    }
    // TODO: set errno
    release(&proc_table.lock);
    return 0;
}

process_t* current_proc() {
    acquire(&proc_table.lock);
    if (proc_table.num_procs == 0) {
        return 0;
    }
    process_t* proc = &proc_table.procs[proc_table.curr_proc];
    release(&proc_table.lock);
    return proc;
}

void copy_context(trap_frame_t* dst, trap_frame_t* src) {
    for (int i = 0; i < 32; i++) {
        dst->regs[i] = src->regs[i];
    }
}

void proc_exit() {
    process_t* proc = current_proc();
    if (proc == 0) {
        // this should never happen, some process called us, right?
        // TODO: panic
        return;
    }
    acquire(&proc->lock);
    release_page(proc->stack_page);
    proc->state = PROC_STATE_AVAILABLE;
    release(&proc->lock);

    acquire(&proc_table.lock);
    proc_table.num_procs--;
    release(&proc_table.lock);
    schedule_user_process();
}
