// Entry points for all the syscalls here

#include "proc.h"
#include "syscalls.h"

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
    [SYS_NR_detach]    sys_detach,
    [SYS_NR_isopen]    sys_isopen,
    [SYS_NR_pipeattch] sys_pipeattch,
    [SYS_NR_lsdir]     sys_lsdir,
};
int syscall_vector_len _text = SYS_NR_lsdir;

regsize_t sys_restart() {
    return proc_restart();
}

regsize_t sys_exit() {
    return proc_exit();
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
    return proc_open(filepath, flags);
}

int32_t sys_close() {
    uint32_t fd = (uint32_t)trap_frame.regs[REG_A0];
    return proc_close(fd);
}

int32_t sys_wait() {
    wait_cond_t *cond = (wait_cond_t*)trap_frame.regs[REG_A0];
    return proc_wait(cond);
}

uint32_t sys_execv() {
    char const* filename = (char const*)trap_frame.regs[REG_A0];
    char const** argv = (char const**)trap_frame.regs[REG_A1];
    return proc_execv(filename, argv);
}

uint32_t sys_getpid() {
    return proc_getpid();
}

uint32_t sys_dup() {
    uint32_t fd = (uint32_t)trap_frame.regs[REG_A0];
    return proc_dup(fd);
}

uint32_t sys_pipe() {
    uint32_t *fds = (uint32_t*)trap_frame.regs[REG_A0];
    return proc_pipe(fds);
}

uint32_t sys_sysinfo() {
    return proc_sysinfo();
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
    return proc_pgalloc();
}

regsize_t sys_pgfree() {
    void *page = (void*)trap_frame.regs[REG_A0];
    return proc_pgfree(page);
}

uint32_t sys_gpio() {
    uint32_t pin_num = (uint32_t)trap_frame.regs[REG_A0];
    uint32_t enable = (uint32_t)trap_frame.regs[REG_A1];
    uint32_t value = (uint32_t)trap_frame.regs[REG_A2];
    return proc_gpio(pin_num, enable, value);
}

uint32_t sys_detach() {
    return proc_detach();
}

int32_t sys_isopen() {
    int32_t fd = (int32_t)trap_frame.regs[REG_A0];
    return proc_isopen(fd);
}

uint32_t sys_pipeattch() {
    uint32_t pid = (uint32_t)trap_frame.regs[REG_A0];
    int32_t src_fd = (int32_t)trap_frame.regs[REG_A1];
    return proc_pipeattch(pid, src_fd);
}

uint32_t sys_lsdir() {
    char const *dir = (char const*)trap_frame.regs[REG_A0];
    dirent_t *dirents = (dirent_t*)trap_frame.regs[REG_A1];
    regsize_t size = trap_frame.regs[REG_A2];
    return proc_lsdir(dir, dirents, size);
}
