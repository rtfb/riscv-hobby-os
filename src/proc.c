#include "proc.h"
#include "pagealloc.h"
#include "programs.h"
#include "kernel.h"
#include "string.h"
#include "drivers/uart/uart.h"

proc_table_t proc_table;
trap_frame_t trap_frame;
cpu_t cpu;

void init_process_table(uint32_t runflags) {
    cpu.proc = 0;
    proc_table.pid_counter = 0;
    for (int i = 0; i < MAX_PROCS; i++) {
        proc_table.procs[i].state = PROC_STATE_AVAILABLE;
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

// sleep_scheduler is called when there's nothing to schedule; this either
// means that something went terribly wrong, or all processes are sleeping. In
// which case we should simply schedule the next timer tick and do nothing.
void sleep_scheduler() {
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);

    // this is almost identical to enable_interrupts, except that it sets MIE
    // flag immediately, instead of setting the MPIE flag. That's because we
    // don't call mret in this code path, which reacts to the pending interrupt
    // enable flag.
    unsigned int mstatus = get_mstatus();
    mstatus |= ((1 << 3) | (1 << 7));
    set_mstatus(mstatus);
    set_mie(1 << 7);

    park_hart();
}

void scheduler() {
    int i_after_wakeup = -1;
    int new_loop = 0;
    while (1) {
        for (int i = 0; i < MAX_PROCS; i++) {
            if (new_loop && i_after_wakeup < i) {
                // we have checked all procs and didn't find anything to run,
                // so enable interrupts and sleep, as there's nothing to
                // schedule now
                sleep_scheduler();
            }
            process_t *p = &proc_table.procs[i];
            acquire(&p->lock);
            if (p->state == PROC_STATE_SLEEPING && should_wake_up(p)) {
                p->state = PROC_STATE_READY;
                // we're waking up the process, which means it's possible that
                // we interrupted sleep_scheduler, which in turn means
                // trap_frame currently corresponds to a sleeping scheduler.
                // Overwrite trap_frame with what the user process has:
                copy_trap_frame(&trap_frame, &p->trap);
                // trap_frame.pc = p->trap.pc;
                // trap_frame.regs[REG_RA] = p->trap.regs[REG_RA];
            }
            if (p->state == PROC_STATE_READY) {
                p->state = PROC_STATE_RUNNING;
                cpu.proc = p;
                // switch context into p. This will not return until p itself
                // does not call swtch().
                swtch(&cpu.context, &p->ctx);
                i_after_wakeup = i;
                new_loop = 0;
                // the process has yielded the cpu, keep looking for something
                // to run
                cpu.proc = 0;
            }
            release(&p->lock);
        }
        new_loop = 1;
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

    process_t* child = alloc_process(sp, ksp);
    if (!child) {
        release_page(ksp);
        release_page(sp);
        return -1;
    }
    child->pid = alloc_pid();
    child->parent = parent;
    child->trap.pc = parent->trap.pc;
    copy_page(child->stack_page, parent->stack_page);
    copy_page(child->kstack_page, parent->kstack_page);
    copy_trap_frame(&child->trap, &parent->trap);
    copy_files(child, parent);

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

void copy_files(process_t *dst, process_t *src) {
    for (int i = 0; i < MAX_PROC_FDS; i++) {
        file_t *pf = src->files[i];
        if (pf) {
            pf->refcount++;
        }
        dst->files[i] = pf;
    }
}

void forkret() {
    process_t* proc = myproc();
    copy_trap_frame(&trap_frame, &proc->trap);
    release(&proc->lock);
    enable_interrupts();
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
    void *top_of_sp = sp + PAGE_SIZE - sizeof(regsize_t);
    sp_argv_t sp_argv = copy_argv(top_of_sp, argc, argv);
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

process_t* alloc_process(void *sp, void *ksp) {
    acquire(&proc_table.lock);
    for (int i = 0; i < MAX_PROCS; i++) {
        process_t *proc = &proc_table.procs[i];
        if (proc == cpu.proc) {
            continue;
        }
        acquire(&proc->lock);
        if (proc->state == PROC_STATE_AVAILABLE) {
            init_proc(proc, sp, ksp);
            release(&proc->lock);
            release(&proc_table.lock);
            return proc;
        }
        release(&proc->lock);
    }
    // TODO: set errno
    release(&proc_table.lock);
    return 0;
}

// must be called with proc_table.lock and proc->lock held.
void init_proc(process_t* proc, void *sp, void *ksp) {
    proc->stack_page = sp;
    proc->kstack_page = ksp;
    proc->state = PROC_STATE_READY;
    for (int i = FD_STDERR + 1; i < MAX_PROC_FDS; i++) {
        proc->files[i] = 0;
    }
    proc->files[FD_STDIN] = &stdin;
    proc->files[FD_STDOUT] = &stdout;
    proc->files[FD_STDERR] = &stderr;
    for (int i = 0; i < 14; i++) {
        proc->ctx.regs[i] = 0;
    }
    proc->ctx.regs[REG_RA] = (regsize_t)forkret;

    // sacrifice the first word of kstack_page for a sentinel that will be used
    // to detect stack overflows on the kernel side:
    proc->magic = (uint32_t*)proc->kstack_page;
    *proc->magic = PROC_MAGIC_STACK_SENTINEL;

    proc_table.num_procs++;
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

void save_sp(regsize_t sp) {
    process_t *p = cpu.proc;
    if (!p) {
        return;
    }
    p->ctx.regs[REG_SP] = sp;
}

void proc_exit() {
    process_t* proc = myproc();
    release_page(proc->stack_page);
    proc->state = PROC_STATE_ZOMBIE;
    acquire(&proc->parent->lock);
    proc->parent->state = PROC_STATE_READY;
    copy_trap_frame(&trap_frame, &proc->parent->trap); // we should return to parent after child exits
    release(&proc->parent->lock);

    for (int i = 0; i < MAX_PROC_FDS; i++) {
        file_t *pf = proc->files[i];
        if (pf != 0) {
            fs_free_file(pf);
        }
    }

    acquire(&proc_table.lock);
    proc_table.num_procs--;
    release(&proc_table.lock);
    swtch(&proc->ctx, &cpu.context);
}

// check_exited_children iterates over process table looking for zombie
// children of a given process. If there are any, the first found is cleaned up
// and its pid is returned. Otherwise, -1 is returned.
int32_t check_exited_children(process_t *proc) {
    for (int i = 0; i < MAX_PROCS; i++) {
        process_t *p = &proc_table.procs[i];
        if (p->parent == proc ) {
            acquire(&p->lock);
            if (p->state == PROC_STATE_ZOMBIE) {
                uint32_t pid = p->pid;
                release_page(p->kstack_page);
                p->state = PROC_STATE_AVAILABLE;
                release(&p->lock);
                return pid;
            }
            release(&p->lock);
        }
    }
    return -1;
}

int32_t wait_or_sleep(uint64_t wakeup_time) {
    process_t* proc = myproc();
    proc->state = PROC_STATE_SLEEPING;
    proc->wakeup_time = wakeup_time;
    copy_trap_frame(&proc->trap, &trap_frame); // save trap context before sleep
    swtch(&proc->ctx, &cpu.context);
    return check_exited_children(proc);
}

void sched() {
    process_t* proc = myproc();
    if (!proc) {
        scheduler(); // no process was scheduled, let the scheduler run, forever
        return; // this is just for clarity: scheduler() never returns
    }
    proc->state = PROC_STATE_READY;
    swtch(&proc->ctx, &cpu.context);
}

int32_t proc_wait() {
    process_t* proc = myproc();
    int32_t chpid = check_exited_children(proc);
    if (chpid >= 0) {
        return chpid;
    }
    return wait_or_sleep(0);
}

void proc_yield(void *chan) {
    process_t* proc = myproc();
    proc->chan = chan;
    wait_or_sleep(0);
    copy_trap_frame(&trap_frame, &proc->trap); // restore trap context after wakeup
}

int32_t proc_sleep(uint64_t milliseconds) {
    uint64_t now = time_get_now();
    uint64_t delta = (ONE_SECOND/1000)*milliseconds;
    return wait_or_sleep(now + delta);
}

void proc_mark_for_wakeup(void *chan) {
    acquire(&proc_table.lock);
    if (proc_table.num_procs == 0) {
        release(&proc_table.lock);
        return;
    }
    for (int i = 0; i < MAX_PROCS; i++) {
        process_t *p = &proc_table.procs[i];
        if (p->state == PROC_STATE_SLEEPING && p->chan == chan) {
            acquire(&p->lock);
            p->state = PROC_STATE_READY;
            p->chan = 0;
            release(&p->lock);
        }
    }
    release(&proc_table.lock);
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
    process_t* self = myproc();
    acquire(&proc_table.lock);
    for (int i = 0; i < MAX_PROCS; i++) {
        process_t *proc = &proc_table.procs[i];
        if (proc->pid == pid) {
            if (pid != self->pid) {
                // only acquire if we're looking up other proc's info,
                // self->pid is already acquired
                acquire(&proc->lock);
            }
            pinfo->pid = proc->pid;
            strncpy(pinfo->name, proc->name, 16);
            pinfo->state = proc->state;
            if (pid != self->pid) {
                release(&proc->lock);
            }
            break;
        }
    }
    release(&proc_table.lock);
    return 0;
}

int32_t fd_alloc(process_t *proc, file_t *f) {
    for (int i = 0; i < MAX_PROC_FDS; i++) {
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
    file_t *f = proc->files[fd];
    if (f == 0) {
        // Reading a non-open file. TODO: set errno
        return -1;
    }
    if (!buf) {
        // TODO: errno
        return -1;
    }
    int32_t nread = fs_read(f, f->position, buf, size);
    if (nread > 0) {
        f->position += nread;
    }
    return nread;
}

int32_t proc_write(uint32_t fd, void *buf, uint32_t nbytes) {
    process_t* proc = myproc();
    file_t *f = proc->files[fd];
    if (f == 0) {
        // Writing a non-open file. TODO: set errno
        return -1;
    }
    if (!buf) {
        // TODO: errno
        return -1;
    }
    if (nbytes == -1) {
        // XXX: this is an ugly hack: I can't always do strlen() in the
        // userland, because the userland currently is unable to access
        // string literals in .rodata.
        nbytes = kstrlen(buf);
    }
    int32_t status = fs_write(f, f->position, buf, nbytes);
    return status;
}

int32_t proc_close(uint32_t fd) {
    process_t* proc = myproc();
    // TODO: flush when we have any buffering
    file_t *f = proc->files[fd];
    if (f == 0) {
        // Closing a non-open file. TODO: set errno
        return -1;
    }
    fs_free_file(f);
    fd_free(proc, fd);
}

int32_t proc_dup(uint32_t fd) {
    process_t* proc = myproc();
    file_t *f = proc->files[fd];
    if (f == 0) {
        // duplicating a non-open file. TODO: set errno
        return -1;
    }
    int32_t newfd = fd_alloc(proc, f);
    if (newfd < 0) {
        // TODO: set errno to indicate out of proc FDs
        return newfd;
    }
    f->refcount++;
    return newfd;
}
