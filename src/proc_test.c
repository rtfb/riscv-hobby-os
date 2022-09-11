#include "proc.h"
#include "kernel.h"
#include "fdt.h"
#include "string.h"
#include "pagealloc.h"
#include "programs.h"

// defined in userland.c:
extern int u_main_init();
extern int u_main();
extern int u_main2();
extern int u_main3();
extern int u_main_shell();
extern int u_main_hello1();
extern int u_main_hello2();
extern int u_main_sysinfo();
extern int u_main_fmt();
extern int u_main_smoke_test();
extern int u_main_hanger();
extern int u_main_ps();
extern int u_main_cat();
extern int u_main_coma();

user_program_t userland_programs[MAX_USERLAND_PROGS] _rodata = {
    (user_program_t){
        .entry_point = &u_main_shell,
        .name = "sh",
    },
    (user_program_t){
        .entry_point = &u_main_hello1,
        .name = "hello1",
    },
    (user_program_t){
        .entry_point = &u_main_hello2,
        .name = "hello2",
    },
    (user_program_t){
        .entry_point = &u_main_sysinfo,
        .name = "sysinfo",
    },
    (user_program_t){
        .entry_point = &u_main_fmt,
        .name = "fmt",
    },
    (user_program_t){
        .entry_point = &u_main_smoke_test,
        .name = "smoke-test",
    },
    (user_program_t){
        .entry_point = &u_main_hanger,
        .name = "hang",
    },
    (user_program_t){
        .entry_point = &u_main_ps,
        .name = "ps",
    },
    (user_program_t){
        .entry_point = &u_main_cat,
        .name = "cat",
    },
    (user_program_t){
        .entry_point = &u_main_coma,
        .name = "coma",
    },
    // keep this last, it's a sentinel:
    (user_program_t){
        .entry_point = 0,
        .name = 0,
    },
};

void init_test_processes() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return;
    }
    if (!strncmp(fdt_get_bootargs(), "smoke-test", ARRAY_LENGTH("smoke-test"))) {
        assign_init_program("smoke-test");
    } else {
        assign_init_program("sh");
    }
}

void assign_init_program(char const* prog) {
    user_program_t *program = find_user_program(prog);
    proc_table.num_procs = 1;
    process_t* p0 = &proc_table.procs[0];
    p0->pid = alloc_pid();
    p0->trap.pc = (regsize_t)program->entry_point;
    p0->name = program->name;
    p0->state = PROC_STATE_READY;
    void* sp = allocate_page();
    if (!sp) {
        // TODO: panic
        return;
    }
    p0->stack_page = sp;
    p0->trap.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);
}

user_program_t* find_user_program(char const *name) {
    for (int i = 0; i < MAX_USERLAND_PROGS; i++) {
        if (userland_programs[i].name && !strncmp(name, userland_programs[i].name, 16)) {
            return &userland_programs[i];
        }
    }
    return 0;
}
