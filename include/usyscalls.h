#ifndef _USYSCALLS_H_
#define _USYSCALLS_H_

// Declarations for userland end of the system calls. Implemented in
// usyscalls.s.

extern void sys_puts(char const* msg);
extern uint32_t getpid();

#endif // ifndef _USYSCALLS_H_
