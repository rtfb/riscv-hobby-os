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

user_program_t userland_programs[MAX_USERLAND_PROGS];

void init_test_processes() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return;
    }

    for (int i = 0; i < MAX_USERLAND_PROGS; i++) {
        userland_programs[i].entry_point = 0;
        userland_programs[i].name = 0;
    }
    userland_programs[0].entry_point = &u_main_shell;
    userland_programs[0].name = "sh";
    userland_programs[1].entry_point = &u_main_hello1;
    userland_programs[1].name = "hello1";
    userland_programs[2].entry_point = &u_main_hello2;
    userland_programs[2].name = "hello2";
    userland_programs[3].entry_point = &u_main_sysinfo;
    userland_programs[3].name = "sysinfo";
    userland_programs[4].entry_point = &u_main_fmt;
    userland_programs[4].name = "fmt";
    userland_programs[5].entry_point = &u_main_smoke_test;
    userland_programs[5].name = "smoke-test";
    userland_programs[6].entry_point = &u_main_hanger;
    userland_programs[6].name = "hang";

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
    p0->context.pc = (regsize_t)program->entry_point;
    p0->name = program->name;
    p0->state = PROC_STATE_READY;
    void* sp = allocate_page();
    if (!sp) {
        // TODO: panic
        return;
    }
    p0->stack_page = sp;
    p0->context.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);
}

user_program_t* find_user_program(char const *name) {
    for (int i = 0; i < MAX_USERLAND_PROGS; i++) {
        if (userland_programs[i].name && !strncmp(name, userland_programs[i].name, 16)) {
            return &userland_programs[i];
        }
    }
    return 0;
}
