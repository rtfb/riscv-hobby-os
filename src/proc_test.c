#include "kernel.h"
#include "pagealloc.h"
#include "proc.h"
#include "programs.h"
#include "runflags.h"
#include "string.h"

user_program_t userland_programs[MAX_USERLAND_PROGS] _rodata = {
    (user_program_t){
        .entry_point = &u_main_shell,
        .name = "sh",
    },
    (user_program_t){
        .entry_point = &u_main_hello,
        .name = "hello",
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
    (user_program_t){
        .entry_point = &u_main_fibd,
        .name = "fibd",
    },
    (user_program_t){
        .entry_point = &u_main_fib,
        .name = "fib",
    },
    (user_program_t){
        .entry_point = &u_main_wait,
        .name = "wait",
    },
    (user_program_t){
        .entry_point = &u_main_ls,
        .name = "ls",
    },
    (user_program_t){
        .entry_point = &u_main_clock,
        .name = "clock",
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
        assign_init_program("sh", test_script);
    } else {
        assign_init_program("sh", 0);
    }
}

void assign_init_program(char const* prog, char const *test_script) {
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
    if (test_script != 0) {
        char const *args[] = {program->name, "-f", test_script};
        inject_argv(p0, 3, args);
    }
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
