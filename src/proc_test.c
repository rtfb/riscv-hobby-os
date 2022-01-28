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
extern int u_main_shell();
extern int u_main_hello1();
extern int u_main_hello2();
extern int u_main_sysinfo();

void* userland_main_funcs[MAX_USERLAND_PROGS];
int num_userland_progs;

void init_test_processes() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return;
    }

    userland_main_funcs[0] = &u_main_init;
    userland_main_funcs[1] = &u_main;
    userland_main_funcs[2] = &u_main2;
    userland_main_funcs[3] = &u_main3;
    userland_main_funcs[4] = &u_main_shell;
    userland_main_funcs[5] = &u_main_hello1;
    userland_main_funcs[6] = &u_main_hello2;
    userland_main_funcs[7] = &u_main_sysinfo;
    num_userland_progs = 8;

    proc_table.num_procs = 1;
    process_t* p0 = &proc_table.procs[0];
    p0->pid = alloc_pid();
    // p0->pc = &u_main_init;
    p0->pc = &u_main_shell;
    p0->state = PROC_STATE_READY;
    void* sp = allocate_page();
    if (!sp) {
        // TODO: panic
        return;
    }
    p0->stack_page = sp;
    p0->context.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);
}
