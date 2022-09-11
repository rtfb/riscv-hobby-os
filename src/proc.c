#include "proc.h"
#include "pagealloc.h"
#include "programs.h"
#include "kernel.h"
#include "string.h"

proc_table_t proc_table;
trap_frame_t trap_frame;

void init_process_table() {
    proc_table.curr_proc = 0;
    proc_table.pid_counter = 0;
    proc_table.is_idle = 1;
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
    uint64_t now = time_get_now();
    acquire(&proc_table.lock);
    int curr_proc = proc_table.curr_proc;
    process_t *last_proc = &proc_table.procs[curr_proc];
    if (last_proc->state == PROC_STATE_AVAILABLE || proc_table.is_idle) {
        // schedule_user_process may have been called from proc_exit, which
        // kills the process in curr_proc slot, so if that's the case,
        // pretend there wasn't any last_proc:
        last_proc = 0;
    }
    if (proc_table.num_procs == 0) {
        release(&proc_table.lock);
        return;
    }

    process_t *proc = find_ready_proc(curr_proc);
    if (!proc) {
        // nothing to schedule; this either means that something went terribly
        // wrong, or all processes are sleeping. In which case we should simply
        // schedule the next timer tick and do nothing
        proc_table.is_idle = 1;
        release(&proc_table.lock);
        set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
        enable_interrupts();
        park_hart();
        return;
    }
    acquire(&proc->lock);
    proc->state = PROC_STATE_RUNNING;

    if (last_proc == 0) {
        copy_trap_frame(&trap_frame, &proc->trap);
    } else if (last_proc->pid != proc->pid) {
        // the user process has changed: save the descending process's context
        // and load the ascending one's
        acquire(&last_proc->lock);
        copy_trap_frame(&last_proc->trap, &trap_frame);
        if (last_proc->state != PROC_STATE_SLEEPING) {
            // restore last_proc to the ready state, but only if it's not sleeping
            last_proc->state = PROC_STATE_READY;
        }
        release(&last_proc->lock);
        copy_trap_frame(&trap_frame, &proc->trap);
    }
    release(&proc->lock);
    proc_table.is_idle = 0;
    release(&proc_table.lock);
    set_user_mode();
}

process_t* find_ready_proc(int curr_proc) {
    int orig_curr_proc = curr_proc;
    process_t* proc = 0;
    do {
        curr_proc++;
        if (curr_proc >= MAX_PROCS) {
            curr_proc = 0;
        }
        proc = &proc_table.procs[curr_proc];
        if (proc->state == PROC_STATE_READY) {
            break;
        }
        if (proc->state == PROC_STATE_SLEEPING && should_wake_up(proc)) {
            proc->state = PROC_STATE_READY;
            break;
        }
    } while (curr_proc != orig_curr_proc);
    proc_table.curr_proc = curr_proc;
    if (proc->state == PROC_STATE_SLEEPING) {
        // this can happen if all processes are sleeping
        return 0;
    }
    return proc;
}

int should_wake_up(process_t* proc) {
    uint64_t now = time_get_now();
    if (proc->wakeup_time != 0 && proc->wakeup_time <= now) {
        return 1;
    }
    return 0;
}

uint32_t proc_fork() {
    // allocate stack. Fail early if we're out of memory:
    void* sp = allocate_page();
    if (!sp) {
        // TODO: set errno
        return -1;
    }

    process_t* parent = myproc();
    acquire(&parent->lock);
    parent->trap.pc = trap_frame.pc;
    copy_trap_frame(&parent->trap, &trap_frame);

    process_t* child = alloc_process();
    if (!child) {
        release_page(sp);
        release(&parent->lock);
        return -1;
    }
    child->pid = alloc_pid();
    child->parent = parent;
    child->trap.pc = parent->trap.pc;
    child->stack_page = sp;
    copy_page(child->stack_page, parent->stack_page);
    copy_trap_frame(&child->trap, &parent->trap);

    // overwrite the sp with the same offset as parent->sp, but within the child stack:
    regsize_t offset = parent->trap.regs[REG_SP] - (regsize_t)parent->stack_page;
    child->trap.regs[REG_SP] = (regsize_t)(sp + offset);
    offset = parent->trap.regs[REG_FP] - (regsize_t)parent->stack_page;
    child->trap.regs[REG_FP] = (regsize_t)(sp + offset);
    // child's return value should be a 0 pid:
    child->trap.regs[REG_A0] = 0;
    release(&parent->lock);
    release(&child->lock);
    trap_frame.regs[REG_A0] = child->pid;
    return child->pid;
}

regsize_t len_argv(char const* argv[]) {
    regsize_t argc = 0;
    if (!argv) {
        return 0;
    }
    while (argv[argc] != 0) {
        argc++;
    }
    return argc;
}

typedef struct {
    regsize_t new_sp;
    regsize_t new_argv;
} sp_argv_t;

// copy_argv takes argv from the calling process and copies it over to the top
// of the stack page of the new process. Returns the new value for sp and argv
// pointing to the new location.
sp_argv_t copy_argv(void *sp, regsize_t argc, char const* argv[]) {
    if (argc == 0 || argv == 0) {
        return (sp_argv_t){
            .new_sp = (regsize_t)sp,
            .new_argv = 0,
        };
    }
    regsize_t* spr = (regsize_t*)sp;
    *spr-- = 0;
    spr -= argc;
    char* spc = (char*)spr;
    spc--;
    int i = 0;
    for (; i < argc; i++) {
        char const* str = argv[i];
        int j = 0;
        while (str[j] != 0) {
            j++;
        }
        *spc-- = 0;
        while (j >= 0) {
            *spc-- = str[j];
            j--;
        }
        spr[i] = (regsize_t)(spc + 1);
    }
    return (sp_argv_t){
        .new_sp = (regsize_t)(spc) & ~7, // down-align at 8, we don't want sp to be odd
        .new_argv = (regsize_t)(spr),
    };
}

uint32_t proc_execv(char const* filename, char const* argv[]) {
    if (filename == 0) {
        // TODO: set errno
        return -1;
    }
    user_program_t *program = find_user_program(filename);
    if (!program) {
        // TODO: set errno
        return -1;
    }
    // allocate stack. Fail early if we're out of memory:
    void* sp = allocate_page();
    if (!sp) {
        // TODO: set errno
        return -1;
    }
    process_t* proc = myproc();
    acquire(&proc->lock);
    proc->trap.pc = (regsize_t)program->entry_point;
    proc->name = program->name;
    release_page(proc->stack_page);
    proc->stack_page = sp;
    regsize_t argc = len_argv(argv);
    sp_argv_t sp_argv = copy_argv(sp + PAGE_SIZE, argc, argv);
    proc->trap.regs[REG_RA] = (regsize_t)proc->trap.pc;
    proc->trap.regs[REG_SP] = sp_argv.new_sp;
    proc->trap.regs[REG_FP] = sp_argv.new_sp;
    proc->trap.regs[REG_A0] = argc;
    proc->trap.regs[REG_A1] = sp_argv.new_argv;
    copy_trap_frame(&trap_frame, &proc->trap);
    release(&proc->lock);
    // syscall() assigns whatever we return here to a0, the register that
    // contains the return value. But in case of exec, we don't really return
    // to the caller, we call the new program's main() instead, so we want a0
    // to contain argc.
    return argc;
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
            return init_proc(&proc_table.procs[i]);
        }
    }
    // TODO: set errno
    release(&proc_table.lock);
    return 0;
}

process_t* init_proc(process_t* proc) {
    acquire(&proc->lock);
    proc->state = PROC_STATE_READY;
    for (int i = 0; i < MAX_PROC_FDS; i++) {
        proc->files[i] = 0;
    }
    proc_table.num_procs++;
    release(&proc_table.lock);
    return proc;
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

process_t* myproc() {
    process_t *proc = current_proc();
    if (!proc) {
        // TODO: panic
        return 0;
    }
    return proc;
}

void copy_trap_frame(trap_frame_t* dst, trap_frame_t* src) {
    for (int i = 0; i < 32; i++) {
        dst->regs[i] = src->regs[i];
    }
}

void proc_exit() {
    process_t* proc = myproc();
    acquire(&proc->lock);
    release_page(proc->stack_page);
    proc->state = PROC_STATE_AVAILABLE;
    acquire(&proc->parent->lock);
    proc->parent->state = PROC_STATE_READY;
    release(&proc->parent->lock);
    release(&proc->lock);

    acquire(&proc_table.lock);
    proc_table.num_procs--;
    release(&proc_table.lock);
    schedule_user_process();
}

int32_t wait_or_sleep(uint64_t wakeup_time) {
    process_t* proc = myproc();
    acquire(&proc->lock);
    proc->state = PROC_STATE_SLEEPING;
    proc->wakeup_time = wakeup_time;
    copy_trap_frame(&proc->trap, &trap_frame); // save the context before sleep
    release(&proc->lock);
    schedule_user_process();
    return 0;
}

int32_t proc_wait() {
    return wait_or_sleep(0);
}

int32_t proc_sleep(uint64_t milliseconds) {
    uint64_t now = time_get_now();
    uint64_t delta = (ONE_SECOND/1000)*milliseconds;
    return wait_or_sleep(now + delta);
}

uint32_t proc_plist(uint32_t *pids, uint32_t size) {
    if (!pids) {
        return -1;
    }
    int p = 0;
    acquire(&proc_table.lock);
    for (int i = 0; i < MAX_PROCS; i++) {
        if (p >= size) {
            // TODO: set errno to indicate that size was too small
            release(&proc_table.lock);
            return -1;
        }
        if (proc_table.procs[i].state != PROC_STATE_AVAILABLE) {
            pids[p] = proc_table.procs[i].pid;
            p++;
        }
    }
    release(&proc_table.lock);
    return p;
}

uint32_t proc_pinfo(uint32_t pid, pinfo_t *pinfo) {
    if (!pinfo) {
        return -1;
    }
    acquire(&proc_table.lock);
    for (int i = 0; i < MAX_PROCS; i++) {
        process_t *proc = &proc_table.procs[i];
        if (proc->pid == pid) {
            acquire(&proc->lock);
            pinfo->pid = proc->pid;
            strncpy(pinfo->name, proc->name, 16);
            pinfo->state = proc->state;
            release(&proc->lock);
            break;
        }
    }
    release(&proc_table.lock);
    return 0;
}

int32_t fd_alloc(process_t *proc, file_t *f) {
    for (int i = FD_STDERR + 1; i < MAX_PROC_FDS; i++) {
        if (proc->files[i] == 0) {
            proc->files[i] = f;
            return i;
        }
    }
    return -1;
}

void fd_free(process_t *proc, int32_t fd) {
    proc->files[fd] = 0;
}

int32_t proc_open(char const *filepath, uint32_t flags) {
    file_t *f = fs_alloc_file();
    if (f == 0) {
        // TODO: set errno to indicate out of global files
        return -1;
    }
    process_t* proc = myproc();
    acquire(&proc->lock);
    int32_t fd = fd_alloc(proc, f);
    if (fd < 0) {
        // TODO: set errno to indicate out of proc FDs
        release(&proc->lock);
        return -1;
    }
    int32_t status = fs_open(f, filepath, flags);
    if (status != 0) {
        proc->files[fd] = 0;
        // TODO: set errno to status
        release(&proc->lock);
        return -1;
    }
    release(&proc->lock);
    return fd;
}

int32_t proc_read(uint32_t fd, void *buf, uint32_t size) {
    process_t* proc = myproc();
    acquire(&proc->lock);
    file_t *f = proc->files[fd];
    release(&proc->lock);
    if (f == 0) {
        // Reading a non-open file. TODO: set errno
        return -1;
    }
    return fs_read(f, f->position, buf, size);
}

int32_t proc_close(uint32_t fd) {
    if (fd <= FD_STDERR) {
        // Can't (yet?) close one of std* fds. TODO: set errno to something useful
        return -1;
    }
    process_t* proc = myproc();
    acquire(&proc->lock);
    // TODO: flush when we have any buffering
    file_t *f = proc->files[fd];
    if (f == 0) {
        // Closing a non-open file. TODO: set errno
        release(&proc->lock);
        return -1;
    }
    fs_free_file(f);
    fd_free(proc, fd);
    release(&proc->lock);
}
