#include "proc.h"
#include "kernel.h"
#include "fdt.h"
#include "string.h"
#include "pagealloc.h"

// defined in userland.c:
extern int u_main_init();
extern int u_main();
extern int u_main2();
extern int u_main3();

void* userland_main_funcs[MAX_USERLAND_PROGS];
int num_userland_progs;

void init_test_processes() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return;
    }
    proc_table.num_procs = 2;
    process_t* p0 = &proc_table.procs[0];
    p0->pid = alloc_pid();
    p0->pc = &u_main;
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
    p1->pc = &u_main2;
    p1->state = PROC_STATE_READY;
    sp = allocate_page();
    if (!sp) {
        // TODO: panic
        return;
    }
    p1->stack_page = sp;
    p1->context.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);

    userland_main_funcs[0] = &u_main_init;
    userland_main_funcs[1] = &u_main;
    userland_main_funcs[2] = &u_main2;
    userland_main_funcs[3] = &u_main3;
    num_userland_progs = 4;
}
