
// The system-specific macros can be listed with this:
// $ riscv64-linux-gnu-gcc -march=rv64g -mabi=lp64 -dM -E - < /dev/null | grep riscv
#if __riscv_xlen == 32
    #define XLEN 32
    #define int64_t long
#elif __riscv_xlen == 64
    #define XLEN 64
    #define int64_t int
#else
    #error Unknown xlen
#endif

extern void sys_puts(char const* msg);
extern int64_t get_clock_cycles();
extern void push_pop_stack();
extern int m_read_hard_id();

extern int *a_string_in_user_mem_ptr;
extern int *msg_m_hello_ptr;
