#ifndef _USERLAND_H_
#define _USERLAND_H_

#include "usyscalls.h"

#define _userland __attribute__((__section__(".user_text")))

extern void push_pop_stack();
extern int *a_string_in_user_mem_ptr;
extern int *msg_m_hello_ptr;

#endif // ifndef _USERLAND_H_
