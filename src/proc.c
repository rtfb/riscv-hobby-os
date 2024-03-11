#include "bakedinfs.h"
#include "drivers/uart/uart.h"
#include "errno.h"
#include "kernel.h"
#include "mem.h"
#include "pagealloc.h"
#include "pmp.h"
#include "proc.h"
#include "programs.h"
#include "string.h"
#include "timer.h"
#include "vm.h"

proc_table_t proc_table;
trap_frame_t trap_frame;
cpu_t cpu;

void init_process_table(uint32_t runflags, unsigned int hart_id) {
    cpu.proc = 0;
    proc_table.pid_counter = 0;
    for (int i = 0; i < MAX_PROCS; i++) {
        proc_table.procs[i].state = PROC_STATE_AVAILABLE;
    }
    init_test_processes(runflags);
    memset(&cpu.context.regs, sizeof(cpu.context.regs), 0);
    cpu.context.regs[REG_SP] = (regsize_t)(&stack_top_addr - hart_id*PAGE_SIZE);
}

// sleep_scheduler is called when there's nothing to schedule; this either
// means that something went terribly wrong, or all processes are sleeping. In
// which case we should simply schedule the next timer tick and do nothing.
void sleep_scheduler() {
#if MIXED_MODE_TIMER
    csr_sip_clear_flags(SIP_SSIP);
#else
    // with MIXED_MODE_TIMER it's advanced in mtimertrap, otherwise we do that here:
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
#endif

    // this is almost identical to enable_interrupts, except that it sets MIE
    // flag immediately, instead of setting the MPIE flag. That's because we
    // don't call OP_xRET in this code path, which does MIE := MPIE atomically,
    // thus only enabling interrupts after OP_xRET actually happens.
    set_status_interrupt_enable_and_pending();
    set_interrupt_enable_bits();

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
            }
            if (p->state == PROC_STATE_READY) {
                p->state = PROC_STATE_RUNNING;
                cpu.proc = p;
                p->nscheds++;
                proc_mark_for_wakeup(p);
                // make sure ret_to_user() returns to p's userland, not to
                // whatever happens to be inside trap_frame now:
                copy_trap_frame(&trap_frame, &p->trap);
                // switch context into p. This will not return until p itself
                // does not call swtch():
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
    process_t* parent = myproc();
    parent->trap.pc = trap_frame.pc;
    copy_trap_frame(&parent->trap, &trap_frame);

    process_t* child = alloc_process();
    if (!child) {
        *parent->perrno = ENOMEM;  // we've probably hit MAX_PROCS, treat it as out of memory
        release(&child->lock);
        return -1;
    }
    uintptr_t status = init_proc(child, parent->trap.pc, 0);
    if (status != 0) {
        *parent->perrno = status;
        release(&child->lock);
        return -1;
    }
    child->parent = parent;
    copy_page(child->stack_page, parent->stack_page);
    copy_page(child->kstack_page, parent->kstack_page);
    copy_trap_frame(&child->trap, &parent->trap);
    copy_files(child, parent);

#if CONFIG_MMU
    // copy the mapping from parent's page tables because the child may be
    // accessing data within that address space (e.g. in order to call exec
    // with the right params).
    copy_page_table(child->upagetable, parent->upagetable, child->pid);
    // remap child stack page again, to overwrite parent stack's page table entry:
    regsize_t guard_page = TOPMOST_VIRT_PAGE - PAGE_SIZE;
    map_page_sv39(child->upagetable, child->stack_page, TOPMOST_VIRT_PAGE, PERM_UDATA, child->pid);
    map_page_sv39(child->upagetable, 0, guard_page, PERM_KDATA, child->pid);
#endif

    // overwrite sp and fp with the same offset as parent's, but within the child stack:
    regsize_t koffset = parent->ctx.regs[REG_SP] - (regsize_t)parent->kstack_page;
    child->ctx.regs[REG_SP] = (regsize_t)(child->kstack_page + koffset);
    child->ctx.regs[REG_FP] = (regsize_t)(child->kstack_page + koffset);
#if !CONFIG_MMU
    // in the presence of MMU, virtual addresses of stacks are identical, so
    // only do reoffset_user_stack when we don't have MMU
    reoffset_user_stack(child, parent, REG_SP);
    reoffset_user_stack(child, parent, REG_FP);
#endif
    // child's return value should be a 0 pid:
    child->trap.regs[REG_A0] = 0;
    release(&child->lock);
    trap_frame.regs[REG_A0] = child->pid;
    return child->pid;
}

// reoffset_user_stack takes a specified register reg from the source process's
// trap frame and calculates an equivalent stack offset in the destination
// process's stack.
regsize_t reoffset_user_stack(process_t *dest, process_t *src, int reg) {
    regsize_t src_stack_page_virt = USR_VIRT(src->stack_page);
    regsize_t offset = src->trap.regs[reg] - src_stack_page_virt;
    dest->trap.regs[reg] = USR_VIRT(dest->stack_page + offset);
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
    regsize_t satp = proc->usatp;
    release(&proc->lock);
    enable_interrupts();
    ret_to_user(satp);
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
sp_argv_t copy_argv(process_t *proc, uintptr_t *sp, regsize_t argc, char const* argv[]) {
    if (argc == 0 || argv == 0) {
        return (sp_argv_t){
            .new_sp = (regsize_t)sp,
            .new_argv = 0,
        };
    }
    *sp-- = 0;
    sp -= argc;
    char* spc = (char*)sp;
    spc--;
    int i = 0;
    for (; i < argc; i++) {
        char const* str = (char const*)va2pa(proc->upagetable, (void*)argv[i]);
        int j = 0;
        while (str[j] != 0) {
            j++;
        }
        *spc-- = 0;
        while (j >= 0) {
            *spc-- = str[j];
            j--;
        }
        sp[i] = USR_STK_VIRT(spc + 1);
    }
    return (sp_argv_t){
        .new_sp = STK_ROUND(spc),
        .new_argv = (regsize_t)(sp),
    };
}

// inject_argv assigns a given argc and argv to a given process. It stores the
// values at the end of the page allocated for stack, pushing the stack pointer
// (and thus, the bottom of the stack) below them. That should only ever be
// done to a freshly initialized process, otherwise the bottom of the stack
// would be ruined. Intended for use in the code path for automated tests.
void inject_argv(process_t *proc, int argc, char const *argv[]) {
    void *stack_bottom = proc->perrno - argc - 1;
    char const **new_argv = stack_bottom;
    for (int i = 0; i < argc; i++) {
        int len = kstrlen(argv[i]);
        stack_bottom -= len + 1;
        strncpy(stack_bottom, argv[i], len + 1);
        *new_argv = (char const*)USR_STK_VIRT(stack_bottom);
        new_argv++;
    }
    *new_argv = 0;
    proc->trap.regs[REG_SP] = STK_ROUND(USR_STK_VIRT(stack_bottom));
    proc->trap.regs[REG_A0] = argc;
    proc->trap.regs[REG_A1] = USR_STK_VIRT(new_argv - argc);
}

uintptr_t* _set_perrno(void *sp) {
    return (uintptr_t*)(sp + user_stack_size) - 1;
}

uint32_t proc_execv(char const* filename, char const* argv[]) {
    process_t* proc = myproc();
    acquire(&proc->lock);
    filename = (char const*)va2pa(proc->upagetable, (void*)filename);
    if (filename == 0) {
        *proc->perrno = EINVAL;
        return -1;
    }
    user_program_t *program = find_user_program(filename);
    if (!program) {
        *proc->perrno = ENOENT;
        return -1;
    }
    // allocate stack. Fail early if we're out of memory:
    void* sp = kalloc("proc_execv: sp", proc->pid); // XXX: we already have a stack_page and kstack_page allocated in fork, do we need a new copy? Why?
    if (!sp) {
        *proc->perrno = ENOMEM;
        return -1;
    }
    proc->trap.pc = USR_VIRT(program->entry_point);
    proc->name = program->name;
    proc->perrno = _set_perrno(sp); // now that we replaced stack_page, update perrno as well
    argv = va2pa(proc->upagetable, argv);
    regsize_t argc = len_argv(argv);
    uintptr_t* top_of_sp = (uintptr_t*)(sp + user_stack_size);
    top_of_sp--;  // compensate for one past the end
    top_of_sp--;  // reserve the last word for errno

    sp_argv_t sp_argv = copy_argv(proc, top_of_sp, argc, argv);

#if CONFIG_MMU
    // map user stack to the top of user address space. Allocate a guard page
    // right below it and map that guard page to null physical page with kernel
    // permissions in order to cause a page fault on stack overflow.
    regsize_t guard_page = TOPMOST_VIRT_PAGE - PAGE_SIZE;
    map_page_sv39(proc->upagetable, sp, TOPMOST_VIRT_PAGE, PERM_UDATA, proc->pid);
    map_page_sv39(proc->upagetable, 0, guard_page, PERM_KDATA, proc->pid);
#endif
    release_page(proc->stack_page);
    proc->stack_page = sp;
    proc->trap.regs[REG_RA] = (regsize_t)proc->trap.pc;
    proc->trap.regs[REG_SP] = USR_STK_VIRT(sp_argv.new_sp);
    proc->trap.regs[REG_FP] = USR_STK_VIRT(sp_argv.new_sp);
    proc->trap.regs[REG_A0] = argc;
    proc->trap.regs[REG_A1] = USR_STK_VIRT(sp_argv.new_argv);
    copy_trap_frame(&trap_frame, &proc->trap);
    proc->procfs_name_file->data = (char*)program->name;
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

// alloc_process finds an available process slot and returns it with proc->lock
// held. It is the caller's responsibility to release it when it's done with
// it. The process is not yet usable, it must be initialized with init_proc
// after that.
process_t* alloc_process() {
    acquire(&proc_table.lock);
    for (int i = 0; i < MAX_PROCS; i++) {
        process_t *proc = &proc_table.procs[i];
        if (proc == cpu.proc) {
            continue;
        }
        acquire(&proc->lock);
        if (proc->state == PROC_STATE_AVAILABLE) {
            release(&proc_table.lock);
            return proc;
        }
        release(&proc->lock);
    }
    release(&proc_table.lock);
    return 0;
}

int32_t procfs_stats_data_func(dq_closure_t *c, char *buf, regsize_t bufsz) {
    process_t *proc = (process_t*)c->data;
    sprintfer_t sprintfer = (sprintfer_t){
        .buf = buf,
        .bufsz = bufsz,
        .fmt = "ppid: %d\nnscheds: %d\nnpages: %d\n",
    };
    uint32_t npages = count_alloced_pages(proc->pid);
    int32_t parent_pid = -1;
    if (proc->parent != 0) {
        parent_pid = proc->parent->pid;
    }
    return ksprintf(&sprintfer, parent_pid, proc->nscheds, npages);
}

// init_proc initializes the given process. Returns 0 on success and error code
// on failure. Must be called with proc->lock held.
uintptr_t init_proc(process_t* proc, regsize_t pc, char const *name) {
    proc->pid = alloc_pid();
    // allocate stack. Fail early if we're out of memory:
    void* sp = kalloc("init_proc: sp", proc->pid);
    if (!sp) {
        return ENOMEM;
    }
    void *ksp = kalloc("init_proc: ksp", proc->pid);
    if (!ksp) {
        release_page(sp);
        return ENOMEM;
    }
#if CONFIG_MMU
    void *upagetable = kalloc("init_proc: upagetable", proc->pid);
    if (!upagetable) {
        release_page(sp);
        release_page(ksp);
        return ENOMEM;
    }
    proc->upagetable = upagetable;
    proc->usatp = MAKE_SATP(upagetable);
    init_user_page_table(upagetable, proc->pid);

    regsize_t guard_page = TOPMOST_VIRT_PAGE - PAGE_SIZE;
    map_page_sv39(proc->upagetable, sp, TOPMOST_VIRT_PAGE, PERM_UDATA, proc->pid);
    map_page_sv39(proc->upagetable, 0, guard_page, PERM_KDATA, proc->pid);
#endif
    proc->name = name;
    proc->trap.pc = pc;
    proc->stack_page = sp;
    proc->perrno = _set_perrno(sp); // reserve the last word for errno
    proc->kstack_page = ksp;
    proc->trap.regs[REG_SP] = STK_ROUND(
        USR_STK_VIRT(sp)    // base of the stack page
        + user_stack_size   // stack size to get to the end
        - sizeof(uintptr_t) // compensate for perrno
    );
    proc->state = PROC_STATE_READY;
    memset(&proc->files, sizeof(proc->files), 0);
    proc->files[FD_STDIN] = &stdin;
    proc->files[FD_STDOUT] = &stdout;
    proc->files[FD_STDERR] = &stderr;
    memset(&proc->ctx.regs, sizeof(proc->ctx.regs), 0);
    proc->ctx.regs[REG_SP] = (regsize_t)ksp + PAGE_SIZE;
    proc->ctx.regs[REG_RA] = (regsize_t)forkret;
    proc->nscheds = 0;
    proc->cond = (pwake_cond_t){
        .type = PWAKE_COND_CHAN,
        .target_pid = -1,
        .want_nscheds = 0,
    };

    // sacrifice the first word of kstack_page for a sentinel that will be used
    // to detect stack overflows on the kernel side:
    proc->magic = (uint32_t*)proc->kstack_page;
    *proc->magic = PROC_MAGIC_STACK_SENTINEL;

    int status = itoa(proc->piddir, MAX_FILENAME_LEN, proc->pid);
    if (status == 0) {
        return ENOBUFS;
    }
    bifs_directory_t *procfs_dir = bifs_mkdir("/proc", proc->piddir);
    if (!procfs_dir) {
        return ENFILE;
    }
    proc->procfs_dir = procfs_dir;
    bifs_file_t *procfs_name_file = bifs_allocate_file();
    if (!procfs_name_file) {
        return ENFILE;
    }
    procfs_name_file->parent = procfs_dir;
    procfs_name_file->flags = BIFS_READABLE | BIFS_RAW;
    procfs_name_file->name = "name";
    procfs_name_file->data = "";
    if (name) {
        procfs_name_file->data = (char*)name;
    }
    proc->procfs_name_file = procfs_name_file;
    bifs_file_t *procfs_stats_file = bifs_allocate_file();
    if (!procfs_stats_file) {
        return ENFILE;
    }
    procfs_stats_file->parent = procfs_dir;
    procfs_stats_file->flags = BIFS_READABLE | BIFS_RAW | BIFS_TMPFILE;
    procfs_stats_file->name = "stats";
    procfs_stats_file->dataquery = (dq_closure_t){
        .func = procfs_stats_data_func,
        .data = proc,
    };

    acquire(&proc_table.lock);
    proc_table.num_procs++;
    release(&proc_table.lock);
    return 0;
}

process_t* current_proc() {
    return cpu.proc;
}

process_t* myproc() {
    process_t *proc = current_proc();
    if (!proc) {
        panic("null in myproc");
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

// patch_proc_sp updates the proc's kernel-side sp. This is needed in
// kernel_timer_tick() because it knows the topmost value of sp before the proc
// gets interrupted. Otherwise, sp gets saved inside swtch(), which is called a
// bit too deep in the call stack and with every timer interrupt the kernel
// stack would slowly erode.
void patch_proc_sp(process_t *proc, regsize_t sp) {
    proc->ctx.regs[REG_SP] = sp;
}

void proc_exit() {
    process_t* proc = myproc();
    release_page(proc->stack_page);
#if CONFIG_MMU
    free_page_table(proc->upagetable);
#endif
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

    proc->procfs_dir->flags = 0;
    proc->procfs_name_file->flags = 0;

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

int32_t psleep(process_t *proc) {
    proc->state = PROC_STATE_SLEEPING;
    copy_trap_frame(&proc->trap, &trap_frame); // save trap context before sleep
    swtch(&proc->ctx, &cpu.context);
    return check_exited_children(proc);
}

void sched() {
    process_t* proc = current_proc();
    if (!proc) {
        scheduler(); // no process was scheduled, let the scheduler run, forever
        return; // this is just for clarity: scheduler() never returns
    }
    proc->state = PROC_STATE_READY;
    swtch(&proc->ctx, &cpu.context);
}

int32_t proc_wait(wait_cond_t *cond) {
    process_t* proc = myproc();
    cond = va2pa(proc->upagetable, cond);
    if (cond) {
        pwake_cond_t pcond = (pwake_cond_t){
            .type = cond->type,
            .target_pid = cond->target_pid,
            .want_nscheds = cond->want_nscheds,
        };
        return proc_wait_by_cond(proc, &pcond);
    }
    int32_t chpid = check_exited_children(proc);
    if (chpid >= 0) {
        return chpid;
    }
    return psleep(proc);
}

int32_t proc_wait_by_cond(process_t *proc, pwake_cond_t *cond) {
    if (cond->type != PWAKE_COND_NSCHEDS) {
        *proc->perrno = ENOSYS;
        return -1;
    }
    process_t *target_proc = find_proc_by_pid(cond->target_pid);
    if (!target_proc) {
        *proc->perrno = ESRCH;
        return -1;
    }
    proc->cond = *cond;
    proc->cond.want_nscheds += target_proc->nscheds;
    proc->chan = target_proc;
    return psleep(proc);
}

void proc_yield(void *chan) {
    process_t* proc = myproc();
    proc->chan = chan;
    psleep(proc);
    copy_trap_frame(&trap_frame, &proc->trap); // restore trap context after wakeup
}

int32_t proc_sleep(uint64_t milliseconds) {
    uint64_t now = time_get_now();
    uint64_t delta = (ONE_SECOND/1000)*milliseconds;
    process_t* proc = myproc();
    proc->wakeup_time = now + delta;
    return psleep(proc);
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
            update_proc_by_chan(p, chan);
            release(&p->lock);
        }
    }
    release(&proc_table.lock);
}

void update_proc_by_chan(process_t *proc, void *chan) {
    if (proc->cond.type == PWAKE_COND_CHAN) {
        proc->state = PROC_STATE_READY;
        proc->chan = 0;
        return;
    }
    if (proc->cond.type == PWAKE_COND_NSCHEDS) {
        process_t *other = (process_t*)chan;
        pwake_cond_t *cond = &proc->cond;
        if (cond->target_pid == other->pid && cond->want_nscheds <= other->nscheds) {
            proc->state = PROC_STATE_READY;
            proc->chan = 0;
        }
        return;
    }
}

uint32_t proc_plist(uint32_t *pids, uint32_t size) {
    process_t* proc = myproc();
    pids = va2pa(proc->upagetable, pids);
    if (!pids) {
        *proc->perrno = EINVAL;
        return -1;
    }
    if (size < MAX_PROCS) {
        *proc->perrno = EINVAL;
        return -1;
    }
    int p = 0;
    acquire(&proc_table.lock);
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proc_table.procs[i].state != PROC_STATE_AVAILABLE) {
            pids[p] = proc_table.procs[i].pid;
            p++;
        }
    }
    release(&proc_table.lock);
    return p;
}

uint32_t proc_pinfo(uint32_t pid, pinfo_t *pinfo) {
    process_t* self = myproc();
    pinfo = va2pa(self->upagetable, pinfo);
    if (!pinfo) {
        return -1;
    }
    acquire(&proc_table.lock);
    process_t *proc = find_proc_by_pid(pid);
    if (proc) {
        if (pid != self->pid) {
            // only acquire if we're looking up other proc's info,
            // self->pid is already acquired
            acquire(&proc->lock);
        }
        pinfo->pid = proc->pid;
        strncpy(pinfo->name, proc->name, 16);
        pinfo->state = proc->state;
        pinfo->nscheds = proc->nscheds;
        if (pid != self->pid) {
            release(&proc->lock);
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
    process_t* proc = myproc();
    filepath = (char const*)va2pa(proc->upagetable, (void*)filepath);
    if (!filepath) {
        *proc->perrno = EINVAL;
        return -1;
    }
    file_t *f = fs_alloc_file();
    if (f == 0) {
        *proc->perrno = ENFILE;
        return -1;
    }
    acquire(&proc->lock);
    int32_t fd = fd_alloc(proc, f);
    if (fd < 0) {
        *proc->perrno = EMFILE;
        release(&proc->lock);
        return -1;
    }
    int32_t status = fs_open(f, filepath, flags);
    if (status < 0) {
        proc->files[fd] = 0;
        *proc->perrno = -status;
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
        *proc->perrno = EBADF;
        return -1;
    }
    buf = va2pa(proc->upagetable, buf);
    if (!buf) {
        *proc->perrno = EINVAL;
        return -1;
    }
    int32_t nread = fs_read(f, f->position, buf, size);
    if (nread < 0) {
        *proc->perrno = -nread;
        return -1;
    }
    f->position += nread;
    return nread;
}

int32_t proc_write(uint32_t fd, void *buf, uint32_t nbytes) {
    process_t* proc = myproc();
    file_t *f = proc->files[fd];
    if (f == 0) {
        *proc->perrno = EBADF;
        return -1;
    }
    buf = va2pa(proc->upagetable, buf);
    if (!buf) {
        *proc->perrno = EINVAL;
        return -1;
    }
    if (nbytes == -1) {
        // XXX: this is an ugly hack: I can't always do strlen() in the
        // userland, because the userland currently is unable to access
        // string literals in .rodata.
        nbytes = kstrlen(buf);
    }
    int32_t status = fs_write(f, f->position, buf, nbytes);
    if (status < 0) {
        *proc->perrno = -status;
        return -1;
    }
    return status;
}

int32_t proc_close(uint32_t fd) {
    process_t* proc = myproc();
    // TODO: flush when we have any buffering
    file_t *f = proc->files[fd];
    if (f == 0) {
        *proc->perrno = EBADF;
        return -1;
    }
    fs_free_file(f);
    fd_free(proc, fd);
}

int32_t proc_dup(uint32_t fd) {
    process_t* proc = myproc();
    file_t *f = proc->files[fd];
    if (f == 0) {
        *proc->perrno = EBADF;
        return -1;
    }
    int32_t newfd = fd_alloc(proc, f);
    if (newfd < 0) {
        *proc->perrno = EMFILE;
        return newfd;
    }
    f->refcount++;
    return newfd;
}

void* proc_pgalloc() {
    process_t* proc = myproc();
    void *page = allocate_page("user", proc->pid, PAGE_USERMEM);
    if (!page) {
        return 0;
    }
#if CONFIG_MMU
    map_page_sv39(proc->upagetable, page, USR_VIRT(page), PERM_UDATA, proc->pid);
#endif
    return (void*)USR_VIRT(page);
}

void proc_pgfree(void *page) {
    process_t* proc = myproc();
    page = va2pa(proc->upagetable, page);
    release_page(page);
}

// proc_detach detaches the current process from its parent. The parent will no
// longer wait() on this child. In effect, this daemonizes the current process.
uint32_t proc_detach() {
    process_t *proc = myproc();
    process_t *parent = proc->parent;
    if (parent == 0) {
        return 0;
    }
    acquire(&proc->lock);
    proc->parent = 0;
    release(&proc->lock);
    // parent may already be wait()ing, so mark it for wakeup:
    acquire(&parent->lock);
    if (parent->state == PROC_STATE_SLEEPING) {
        parent->state = PROC_STATE_READY;
    }
    release(&parent->lock);
    return 0;
}

process_t* find_proc_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCS; i++) {
        process_t *proc = &proc_table.procs[i];
        if (proc->pid == pid) {
            return proc;
        }
    }
    return 0;
}

// proc_pipeattch attaches a file object referred to by a given src_fd to the
// target pid's stdout.
uint32_t proc_pipeattch(uint32_t pid, int32_t src_fd) {
    process_t *proc = myproc();
    acquire(&proc->lock);
    file_t *f = proc->files[src_fd];
    if (!f) {
        *proc->perrno = EBADF;
        return -1;
    }
    f->refcount++;
    release(&proc->lock);
    process_t *target_proc = find_proc_by_pid(pid);
    if (!target_proc) {
        f->refcount--;
        *proc->perrno = ESRCH;
        return -1;
    }
    acquire(&target_proc->lock);
    if (target_proc->files[FD_STDOUT] != 0) {
        f->refcount--;
        release(&target_proc->lock);
        *proc->perrno = EBUSY;
        return -1;
    }
    target_proc->files[FD_STDOUT] = f;
    target_proc->chan = f->fs_file;
    release(&target_proc->lock);
    return 0;
}

int32_t proc_isopen(int32_t fd) {
    process_t *proc = myproc();
    return proc->files[fd] != 0;
}

uint32_t proc_lsdir(char const *dir, dirent_t *dirents, regsize_t size) {
    process_t *proc = myproc();
    dir = va2pa(proc->upagetable, (void*)dir);
    dirents = va2pa(proc->upagetable, dirents);
    bifs_directory_t *parent;
    int32_t status = bifs_opendirpath(&parent, dir, kstrlen(dir));
    if (status != 0) {
        *proc->perrno = -status;
        return -1;
    }
    if (!parent) {
        *proc->perrno = ENOENT;
        return -1;
    }
    status = parent->lsdir(parent, dirents, size);
    if (status < 0) {
        *proc->perrno = -status;
        return -1;
    }
    return status;
}
