// Entry points for all the syscalls here

#include "kernel.h"
#include "syscalls.h"
#include "proc.h"
#include "uart.h"

// for fun let's pretend syscall table is kinda like 32bit Linux on x86,
// /usr/include/asm/unistd_32.h: __NR_restart_syscall 0, __NR_exit 1, _NR_fork 2, __NR_read 3, __NR_write 4
//
// Note that we place syscall_vector in a .text segment in order to have it in
// ROM, since it's read-only after all. We also specify the section with a
// trailing # in order to avoid a warning from GCC, see this answer:
// https://stackoverflow.com/a/58455496/6763
void __attribute__((__section__(".text#"))) *syscall_vector[] = {
    [SYS_NR_restart]   sys_restart,
    [SYS_NR_exit]      sys_exit,
    [SYS_NR_fork]      sys_fork,
    [SYS_NR_read]      sys_read,
    [SYS_NR_write]     sys_write,
    [SYS_NR_wait]      sys_wait,
    [SYS_NR_execv]     sys_execv,
    [SYS_NR_getpid]    sys_getpid,
};

void syscall() {
    int nr = trap_frame.regs[REG_A7];
    if (nr >= 0 && nr < ARRAY_LENGTH(syscall_vector) && syscall_vector[nr] != 0) {
        int32_t (*funcPtr)(void) = syscall_vector[nr];
        trap_frame.regs[REG_A0] = (*funcPtr)();
    } else {
        kprintf("BAD sycall %d\n", nr);
        trap_frame.regs[REG_A0] = -1;
    }
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

int32_t sys_read(uint32_t fd, char* buf, uint32_t bufsize) {
    // ignore fd and default to reading from UART for now, as there's nowhere
    // else to read from
    int32_t nread = 0;
    for (;;) {
        if (nread >= bufsize) {
            break;
        }
        char ch = uart_readchar();
        buf[nread] = ch;
        nread++;
        uart_writechar(ch); // echo back to console
        if (ch == '\r') {
            uart_writechar('\n'); // echo back to console
            break;
        }
    }
    return nread;
}

void sys_write() {
    prints();
}

int32_t sys_wait() {
    return proc_wait();
}

uint32_t sys_execv(char const* filename, char const* argv[]) {
    return proc_execv(filename, argv);
}

uint32_t sys_getpid() {
    acquire(&proc_table.lock);
    uint32_t pid = proc_table.procs[proc_table.curr_proc].pid;
    release(&proc_table.lock);
    return pid;
}
