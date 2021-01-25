
#include <stdint.h>

extern void sys_puts(char const* msg);
extern int64_t get_clock_cycles();
extern void push_pop_stack();
extern int64_t m_read_hard_id();

extern int64_t *a_string_in_user_mem_ptr;
extern int64_t *msg_m_hello_ptr;
