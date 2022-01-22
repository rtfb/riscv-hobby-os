#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include "sys.h"

void sys_restart();
void sys_exit();
uint32_t sys_fork();
void sys_read();
void sys_write();
uint32_t sys_execv();
uint32_t sys_getpid();

void sys_placeholder();

// These are implemented in assembler as of now:
extern void prints();
extern void poweroff();

#endif // ifndef _SYSCALLS_H_
