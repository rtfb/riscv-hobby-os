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
    process_t* p0 = &proc_table.procs[0];
    p0->pid = alloc_pid();
    p0->pc = user_entry_point;
    p0->state = PROC_STATE_READY;
    void* sp = allocate_page();
    if (!sp) {
        // TODO: panic
        return;
    }
    p0->stack_page = sp;
    p0->context.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);

    process_t* p1 = &proc_table.procs[1];
    p1->pid = alloc_pid();
    p1->pc = user_entry_point2;
    p1->state = PROC_STATE_READY;
    sp = allocate_page();
    if (!sp) {
        // TODO: panic
        return;
    }
    p1->stack_page = sp;
    p1->context.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);
}
