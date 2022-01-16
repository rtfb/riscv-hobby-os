#ifndef _USYSCALLS_H_
#define _USYSCALLS_H_

// Declarations for userland end of the system calls. Implemented in
// usyscalls.s.

// TODO: rename sys_puts to write()
extern void sys_puts(char const* msg);
extern uint32_t getpid();
extern uint32_t fork();

#endif // ifndef _USYSCALLS_H_
