#ifndef _USYSCALLS_H_
#define _USYSCALLS_H_

#include "syscalls.h"

// Declarations for userland end of the system calls. Implemented in
// usyscalls.s.

extern void exit();
extern uint32_t fork();
extern int32_t read(uint32_t fd, char* buf, uint32_t bufsize);
// TODO: rename sys_puts to write()
extern void sys_puts(char const* msg);
extern int32_t wait();
extern uint32_t execv(char const* filename, char const* argv[]);
extern uint32_t getpid();
extern uint32_t sysinfo(sysinfo_t* info);

#endif // ifndef _USYSCALLS_H_
