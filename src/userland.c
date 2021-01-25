#include "sys.h"

int __attribute__((__section__(".user_text"))) u_main() {
    sys_puts("Hello from U-mode!\n");

    sys_puts("Read user CSR: ");
    int64_t clock_cycles = get_clock_cycles();
    sys_puts("OK\n");

    sys_puts("Read from memory: ");
    int64_t word = *a_string_in_user_mem_ptr;
    sys_puts("OK\n");

    sys_puts("Read from unaligned memory: ");
    char *cptr = (char*)a_string_in_user_mem_ptr;
    cptr++;
    char c = *cptr;
    sys_puts("OK\n");

    sys_puts("Read/write stack: ");
    push_pop_stack();
    sys_puts("OK\n");

    /*
    TODO: port this bit
    sys_print msg_write_to_readonly_mem
    la      a0, user_entry_point
    sx      t0, 0,(a0)
    sys_print msg_ok
    */

    sys_puts("Illegal read from protected CSR: ");
    int64_t hart_id = m_read_hard_id(); // causes Illegal instruction (mcause=2) in User mode

    sys_puts("Illegal read from protected memory: ");
    word = *msg_m_hello_ptr; // causes Load access fault (mcause=5) in User mode

    sys_puts("C Done.\n");
    return 0;
}
