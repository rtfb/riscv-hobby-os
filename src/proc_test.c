#include "proc.h"
#include "kernel.h"
#include "fdt.h"
#include "string.h"
#include "pagealloc.h"

void init_test_processes() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return;
    }
    proc_table.num_procs = 2;
    proc_table.procs[0].pid = 0;
    proc_table.procs[0].pc = user_entry_point;
    void* sp = allocate_page();
    if (!sp) {
        // TODO: panic
        return;
    }
    proc_table.procs[0].context.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);
    proc_table.procs[1].pid = 1;
    proc_table.procs[1].pc = user_entry_point2;
    sp = allocate_page();
    if (!sp) {
        // TODO: panic
        return;
    }
    proc_table.procs[1].context.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);
}
