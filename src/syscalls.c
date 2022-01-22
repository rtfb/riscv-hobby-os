// Entry points for all the syscalls here

#include "syscalls.h"
#include "proc.h"

// for fun let's pretend syscall table is kinda like 32bit Linux on x86,
// /usr/include/asm/unistd_32.h: __NR_restart_syscall 0, __NR_exit 1, _NR_fork 2, __NR_read 3, __NR_write 4
//
// Note that we place syscall_vector in a .text segment in order to have it in
// ROM, since it's read-only after all. We also specify the section with a
// trailing # in order to avoid a warning from GCC, see this answer:
// https://stackoverflow.com/a/58455496/6763
void __attribute__((__section__(".text#"))) *syscall_vector[] = {
/*  0 */     sys_restart,
/*  1 */     sys_exit,
/*  2 */     sys_fork,
/*  3 */     sys_read,
/*  4 */     sys_write,
/*  5 */     sys_placeholder,
/*  6 */     sys_placeholder,
/*  7 */     sys_placeholder,
/*  8 */     sys_placeholder,
/*  9 */     sys_placeholder,
/* 10 */     sys_placeholder,
/* 11 */     sys_placeholder,
/* 12 */     sys_placeholder,
/* 13 */     sys_placeholder,
/* 14 */     sys_placeholder,
/* 15 */     sys_placeholder,
/* 16 */     sys_placeholder,
/* 17 */     sys_placeholder,
/* 18 */     sys_placeholder,
/* 19 */     sys_placeholder,
/* 20 */     sys_getpid,
};

void sys_restart() {
    poweroff();
}

void sys_exit() {
    proc_exit();
}

uint32_t sys_fork() {
    return proc_fork();
}

void sys_read() {
    // TODO: implement
}

void sys_write() {
    prints();
}

uint32_t sys_getpid() {
    acquire(&proc_table.lock);
    uint32_t pid = proc_table.procs[proc_table.curr_proc].pid;
    release(&proc_table.lock);
    trap_frame.regs[REG_A0] = pid;
    return pid;
}

void sys_placeholder() {
}
