#include "kernel.h"
#include "pagealloc.h"
#include "proc.h"
#include "programs.h"
#include "runflags.h"
#include "string.h"

// defined in userland.c:
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
extern int u_main_pipe();
extern int u_main_pipe2();
extern int u_main_wc();
extern int u_main_gpio();
extern int u_main_iter();
extern int u_main_test_printf();

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
    (user_program_t){
        .entry_point = &u_main_pipe,
        .name = "pp",
    },
    (user_program_t){
        .entry_point = &u_main_pipe2,
        .name = "pp2",
    },
    (user_program_t){
        .entry_point = &u_main_wc,
        .name = "wc",
    },
    (user_program_t){
        .entry_point = &u_main_gpio,
        .name = "gpio",
    },
    (user_program_t){
        .entry_point = &u_main_iter,
        .name = "iter",
    },
    (user_program_t){
        .entry_point = &u_main_test_printf,
        .name = "testprintf",
    },
    // keep this last, it's a sentinel:
    (user_program_t){
        .entry_point = 0,
        .name = 0,
    },
};

void init_test_processes(uint32_t runflags) {
    if (runflags == RUNFLAGS_DRY_RUN) {
        return;
    }
    if (runflags == RUNFLAGS_SMOKE_TEST || runflags == RUNFLAGS_TINY_STACK) {
        assign_init_program("smoke-test");
    } else {
        assign_init_program("sh");
    }
}

void assign_init_program(char const* prog) {
    user_program_t *program = find_user_program(prog);
    if (!program) {
        panic("no init program");
        return;
    }
    process_t* p0 = alloc_process();
    if (!p0) {
        panic("alloc p0 process");
        return;
    }
    uintptr_t status = init_proc(p0, USR_VIRT(program->entry_point), program->name);
    release(&p0->lock);
    if (status != 0) {
        panic("init p0 process");
    }
    cpu.proc = p0;
}

user_program_t* find_user_program(char const *name) {
    for (int i = 0; i < MAX_USERLAND_PROGS; i++) {
        if (userland_programs[i].name && !strncmp(name, userland_programs[i].name, 16)) {
            return &userland_programs[i];
        }
    }
    return 0;
}
