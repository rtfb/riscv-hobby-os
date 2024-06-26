// This file is generated by scripts/gen-syscalls.py from include/syscalls.hh.
// To add a new system call, edit syscalls.hh. gen-syscalls.py will be rerun on
// the next build and this file will be modified. Commit in the changed file for
// easier readability.
#ifndef _SYSCALLDECLS_H_
#define _SYSCALLDECLS_H_

regsize_t sys_exit();
regsize_t sys_fork();
regsize_t sys_read();
regsize_t sys_write();
regsize_t sys_open();
regsize_t sys_close();
regsize_t sys_wait();
regsize_t sys_execv();
regsize_t sys_getpid();
regsize_t sys_dup();
regsize_t sys_pipe();
regsize_t sys_sysinfo();
regsize_t sys_sleep();
regsize_t sys_plist();
regsize_t sys_pinfo();
regsize_t sys_pgalloc();
regsize_t sys_pgfree();
regsize_t sys_gpio();
regsize_t sys_detach();
regsize_t sys_isopen();
regsize_t sys_pipeattch();
regsize_t sys_lsdir();
#endif
