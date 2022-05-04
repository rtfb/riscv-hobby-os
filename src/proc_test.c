#include "proc.h"
#include "kernel.h"
#include "fdt.h"
#include "string.h"

void init_test_processes() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return;
    }
    num_procs = 2;
    proc_table[0].pid = 0;
    proc_table[0].pc = user_entry_point;
    // For now, initialize userland stacks to hardcoded locations within the
    // stack space: proc[0] will have its stack 128*XLEN_BYTES above
    // stack_bottom
    proc_table[0].context.regs[REG_SP] = (regsize_t)(&stack_bottom + 128);
    proc_table[1].pid = 1;
    proc_table[1].pc = user_entry_point2;
    // proc[1] will have its stack 128*XLEN_BYTES above proc[0]'s stack
    proc_table[1].context.regs[REG_SP] = (regsize_t)(&stack_bottom + 256);
}
