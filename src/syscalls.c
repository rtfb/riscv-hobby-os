// Entry points for all the syscalls here

#include "kernel.h"
#include "syscalls.h"
#include "proc.h"
#include "pagealloc.h"
#include "pipe.h"
#include "gpio.h"

// for fun let's pretend syscall table is kinda like 32bit Linux on x86,
// /usr/include/asm/unistd_32.h: __NR_restart_syscall 0, __NR_exit 1, _NR_fork 2, __NR_read 3, __NR_write 4
//
// Note that we place syscall_vector in a .text segment in order to have it in
// ROM, since it's read-only after all.
void *syscall_vector[] _text = {
    [SYS_NR_restart]   sys_restart,
    [SYS_NR_exit]      sys_exit,
    [SYS_NR_fork]      sys_fork,
    [SYS_NR_read]      sys_read,
    [SYS_NR_write]     sys_write,
    [SYS_NR_open]      sys_open,
    [SYS_NR_close]     sys_close,
    [SYS_NR_wait]      sys_wait,
    [SYS_NR_execv]     sys_execv,
    [SYS_NR_getpid]    sys_getpid,
    [SYS_NR_dup]       sys_dup,
    [SYS_NR_pipe]      sys_pipe,
    [SYS_NR_sysinfo]   sys_sysinfo,
    [SYS_NR_sleep]     sys_sleep,
    [SYS_NR_plist]     sys_plist,
    [SYS_NR_pinfo]     sys_pinfo,
    [SYS_NR_pgalloc]   sys_pgalloc,
    [SYS_NR_pgfree]    sys_pgfree,
    [SYS_NR_gpio]      sys_gpio,
};

void syscall() {
    disable_interrupts();
    int nr = trap_frame.regs[REG_A7];
    trap_frame.pc += 4; // step over the ecall instruction that brought us here
    if (trap_frame.regs[REG_SP] < (regsize_t)cpu.proc->stack_page) {
        kprintf("STACK OVERFLOW in userland before pid:syscall %d:%d\n", cpu.proc->pid, nr);
        trap_frame.regs[REG_A0] = -1;
        return;
    }
    if (nr >= 0 && nr < ARRAY_LENGTH(syscall_vector) && syscall_vector[nr] != 0) {
        int32_t (*funcPtr)(void) = syscall_vector[nr];
        trap_frame.regs[REG_A0] = (*funcPtr)();
    } else {
        kprintf("BAD pid:syscall %d:%d\n", cpu.proc->pid, nr);
        trap_frame.regs[REG_A0] = -1;
    }
    if (*cpu.proc->magic != PROC_MAGIC_STACK_SENTINEL) {
        kprintf("STACK OVERFLOW in kernel pid:syscall %d:%d (magic=0x%x)\n",
            cpu.proc->pid, nr, *cpu.proc->magic);
        trap_frame.regs[REG_A0] = -1;
        // TODO: panic
        return;
    }
    enable_interrupts();

    // we might have gotten here from an interrupt that occurred in M-/S-mode,
    // so ensure we will go back to U-mode, not back to one of the privileged
    // ones:
    set_user_mode();
}

void sys_restart() {
    poweroff();
}

void sys_exit() {
    proc_exit();
}

uint32_t sys_fork() {
    return proc_fork();
}

int32_t sys_read() {
    uint32_t fd = (uint32_t)trap_frame.regs[REG_A0];
    void *buf = (void*)trap_frame.regs[REG_A1];
    uint32_t size = (uint32_t)trap_frame.regs[REG_A2];
    return proc_read(fd, buf, size);
}

int32_t sys_write() {
    uint32_t fd = (uint32_t)trap_frame.regs[REG_A0];
    void *buf = (void*)trap_frame.regs[REG_A1];
    uint32_t size = (uint32_t)trap_frame.regs[REG_A2];
    return proc_write(fd, buf, size);
}

int32_t sys_open() {
    char const *filepath = (char const*)trap_frame.regs[REG_A0];
    uint32_t flags = (uint32_t)trap_frame.regs[REG_A1];
    if (!filepath) {
        // TODO: errno
        return -1;
    }
    return proc_open(filepath, flags);
}

int32_t sys_close() {
    uint32_t fd = (uint32_t)trap_frame.regs[REG_A0];
    return proc_close(fd);
}

int32_t sys_wait() {
    return proc_wait();
}

uint32_t sys_execv() {
    char const* filename = (char const*)trap_frame.regs[REG_A0];
    char const** argv = (char const**)trap_frame.regs[REG_A1];
    return proc_execv(filename, argv);
}

uint32_t sys_getpid() {
    uint32_t pid = cpu.proc->pid;
    return pid;
}

uint32_t sys_dup() {
    uint32_t fd = (uint32_t)trap_frame.regs[REG_A0];
    return proc_dup(fd);
}

uint32_t sys_pipe() {
    uint32_t *fds = (uint32_t*)trap_frame.regs[REG_A0];
    return pipe_open(fds);
}

uint32_t sys_sysinfo() {
    sysinfo_t* info = (sysinfo_t*)trap_frame.regs[REG_A0];
    acquire(&proc_table.lock);
    info->procs = proc_table.num_procs;
    release(&proc_table.lock);

    acquire(&paged_memory.lock);
    info->totalram = paged_memory.num_pages;
    info->freeram = count_free_pages();
    info->unclaimed_start = paged_memory.unclaimed_start;
    info->unclaimed_end = paged_memory.unclaimed_end;
    release(&paged_memory.lock);
    return 0;
}

uint32_t sys_sleep() {
    uint32_t milliseconds = (uint32_t)trap_frame.regs[REG_A0];
    return proc_sleep(milliseconds);
}

uint32_t sys_plist() {
    uint32_t *pids = (uint32_t*)trap_frame.regs[REG_A0];
    uint32_t size = (uint32_t)trap_frame.regs[REG_A1];
    return proc_plist(pids, size);
}

uint32_t sys_pinfo() {
    uint32_t pid = (uint32_t)trap_frame.regs[REG_A0];
    pinfo_t *pinfo = (pinfo_t*)trap_frame.regs[REG_A1];
    return proc_pinfo(pid, pinfo);
}

regsize_t sys_pgalloc() {
    return (regsize_t)allocate_page();
}

regsize_t sys_pgfree() {
    void *page = (void*)trap_frame.regs[REG_A0];
    release_page(page);
    return 0;
}

uint32_t sys_gpio() {
    uint32_t pin_num = (uint32_t)trap_frame.regs[REG_A0];
    uint32_t enable = (uint32_t)trap_frame.regs[REG_A1];
    uint32_t value = (uint32_t)trap_frame.regs[REG_A2];
    return gpio_do_syscall(pin_num, enable, value);
}
