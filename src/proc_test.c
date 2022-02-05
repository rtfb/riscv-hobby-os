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

user_program_t userland_programs[MAX_USERLAND_PROGS];
int num_userland_progs;

void init_test_processes() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return;
    }

    userland_programs[0].entry_point = &u_main_init;
    userland_programs[0].name = "init";
    userland_programs[1].entry_point = &u_main;
    userland_programs[1].name = "main1";
    userland_programs[2].entry_point = &u_main2;
    userland_programs[2].name = "main2";
    userland_programs[3].entry_point = &u_main3;
    userland_programs[3].name = "main3";
    userland_programs[4].entry_point = &u_main_shell;
    userland_programs[4].name = "sh";
    userland_programs[5].entry_point = &u_main_hello1;
    userland_programs[5].name = "hello1";
    userland_programs[6].entry_point = &u_main_hello2;
    userland_programs[6].name = "hello2";
    userland_programs[7].entry_point = &u_main_sysinfo;
    userland_programs[7].name = "sysinfo";
    userland_programs[8].entry_point = &u_main_fmt;
    userland_programs[8].name = "fmt";
    num_userland_progs = 9;

    user_program_t *shell = find_user_program("sh");
    proc_table.num_procs = 1;
    process_t* p0 = &proc_table.procs[0];
    p0->pid = alloc_pid();
    p0->pc = shell->entry_point;
    p0->name = shell->name;
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
    for (int i = 0; i < num_userland_progs; i++) {
        if (!strncmp(name, userland_programs[i].name, 16)) {
            return &userland_programs[i];
        }
    }
    return 0;
}
