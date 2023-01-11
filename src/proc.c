#include "proc.h"
#include "pagealloc.h"
#include "programs.h"
#include "kernel.h"
#include "string.h"

proc_table_t proc_table;
trap_frame_t trap_frame;
cpu_t cpu;

void init_process_table(uint32_t runflags) {
    cpu.proc = 0;
    proc_table.pid_counter = 0;
    proc_table.is_idle = 1;
    for (int i = 0; i < MAX_PROCS; i++) {
        proc_table.procs[i].state = PROC_STATE_AVAILABLE;
        proc_table.procs[i].ctx.regs[REG_RA] = (regsize_t)forkret;
    }
    init_test_processes(runflags);

    for (int i = 0; i < 14; i++) {
        cpu.context.regs[i] = 0;
    }
    unsigned int hart_id = get_mhartid();
    cpu.context.regs[REG_SP] = (regsize_t)(&stack_top - hart_id*PAGE_SIZE);
}

void init_global_trap_frame() {
    set_mscratch(&trap_frame);
}

void scheduler() {
    while (1) {
        for (int i = 0; i < MAX_PROCS; i++) {
            process_t *p = &proc_table.procs[i];
            acquire(&p->lock);
            if (p->state == PROC_STATE_READY) {
                p->state = PROC_STATE_RUNNING;
                cpu.proc = p;
                // switch context into p. This will not return until p itself
                // does not call swtch().
                swtch(&cpu.context, &p->ctx);
                // the process has yielded the cpu, keep looking for something
                // to run
                cpu.proc = 0;
            }
            release(&p->lock);
        }
    }
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
    void *ksp = allocate_page();
    if (!ksp) {
        // TODO: set errno
        return -1;
    }

    process_t* parent = myproc();
    parent->trap.pc = trap_frame.pc;
    copy_trap_frame(&parent->trap, &trap_frame);

    process_t* child = alloc_process();
    if (!child) {
        release_page(ksp);
        release_page(sp);
        return -1;
    }
    child->pid = alloc_pid();
    child->parent = parent;
    child->trap.pc = parent->trap.pc;
    child->stack_page = sp;
    child->kstack_page = ksp;
    copy_page(child->stack_page, parent->stack_page);
    copy_page(child->kstack_page, parent->kstack_page);
    copy_trap_frame(&child->trap, &parent->trap);

    // overwrite the sp with the same offset as parent->sp, but within the child stack:
    regsize_t offset = parent->trap.regs[REG_SP] - (regsize_t)parent->stack_page;
    regsize_t koffset = parent->ctx.regs[REG_SP] - (regsize_t)parent->kstack_page;
    child->trap.regs[REG_SP] = (regsize_t)(sp + offset);
    child->ctx.regs[REG_SP] = (regsize_t)(ksp + koffset);
    offset = parent->trap.regs[REG_FP] - (regsize_t)parent->stack_page;
    child->trap.regs[REG_FP] = (regsize_t)(sp + offset);
    child->ctx.regs[REG_FP] = (regsize_t)(ksp + koffset);
    // child's return value should be a 0 pid:
    child->trap.regs[REG_A0] = 0;
    release(&child->lock);
    trap_frame.regs[REG_A0] = child->pid;
    return child->pid;
}

void forkret() {
    process_t* proc = myproc();
    copy_trap_frame(&trap_frame, &proc->trap);
    release(&proc->lock);
    ret_to_user();
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
    void* sp = allocate_page(); // XXX: we already have a stack_page and kstack_page allocated in fork, do we need a new copy? Why?
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
        process_t *proc = &proc_table.procs[i];
        if (proc == cpu.proc) {
            continue;
        }
        if (proc->state == PROC_STATE_AVAILABLE) {
            return init_proc(proc); // will release proc_table.lock
        }
    }
    // TODO: set errno
    release(&proc_table.lock);
    return 0;
}

// must be called with proc_table.lock held and is responsible for releasing it.
process_t* init_proc(process_t* proc) {
    acquire(&proc->lock);
    proc->state = PROC_STATE_READY;
    for (int i = 0; i < MAX_PROC_FDS; i++) {
        proc->files[i] = 0;
    }
    for (int i = 0; i < 14; i++) {
        proc->ctx.regs[i] = 0;
    }
    proc->ctx.regs[REG_RA] = (regsize_t)forkret;
    proc_table.num_procs++;
    release(&proc_table.lock);
    return proc;
}

process_t* current_proc() {
    return cpu.proc;
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

void copy_context(context_t *dst, context_t *src) {
    for (int i = 0; i < 14; i++) {
        dst->regs[i] = src->regs[i];
    }
}

void proc_exit() {
    process_t* proc = myproc();
    release_page(proc->stack_page);
    release_page(proc->kstack_page);   // XXX: problem! Can't release kstack_page yet, until we swtch outta here
    proc->state = PROC_STATE_AVAILABLE;
    acquire(&proc->parent->lock);
    proc->parent->state = PROC_STATE_READY;
    copy_trap_frame(&trap_frame, &proc->parent->trap); // we should return to parent after child exits
    release(&proc->parent->lock);

    acquire(&proc_table.lock);
    proc_table.num_procs--;
    release(&proc_table.lock);
    swtch(&proc->ctx, &cpu.context);
}

int32_t wait_or_sleep(uint64_t wakeup_time) {
    process_t* proc = myproc();
    proc->state = PROC_STATE_SLEEPING;
    proc->wakeup_time = wakeup_time;
    copy_trap_frame(&proc->trap, &trap_frame); // save trap context before sleep
    swtch(&proc->ctx, &cpu.context);
    return 0;
}

void sched() {
    process_t* proc = myproc();
    acquire(&proc->lock);
    swtch(&proc->ctx, &cpu.context);
    release(&proc->lock);
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
