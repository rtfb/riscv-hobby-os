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

    sys_puts("Illegal write to read-only memory: ");
    // XXX: this ought to fail, but succeeds in QEMU for some reason:
    *a_string_in_user_mem_ptr = 7;
    sys_puts("OK\n");

    sys_puts("Illegal read from protected CSR: ");
    // causes Illegal instruction (mcause=2) in User mode:
    int64_t hart_id = m_read_hard_id();

    sys_puts("Illegal read from protected memory: ");
    // causes Load access fault (mcause=5) in User mode:
    word = *msg_m_hello_ptr;

    sys_puts("C Done.\n");
    return 0;
}
