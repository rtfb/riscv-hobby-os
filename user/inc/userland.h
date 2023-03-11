#ifndef _USERLAND_H_
#define _USERLAND_H_

#include "usyscalls.h"

#define _userland __attribute__((__section__(".user_text")))
#define _user_rodata __attribute__((__section__(".user_rodata")))

extern void push_pop_stack();
extern int *a_string_in_user_mem_ptr;
extern int *msg_m_hello_ptr;

// defined in user-printf.s
extern int32_t _userland printf(char const* fmt, ...);

// defined in user-printf.c
extern int32_t _userland prints(char const* str);

#endif // ifndef _USERLAND_H_
