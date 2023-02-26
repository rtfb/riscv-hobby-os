#include "proc.h"
#include "kernel.h"
#include "string.h"
#include "pagealloc.h"
#include "programs.h"
#include "runflags.h"

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
extern int u_main_pipe();
extern int u_main_pipe2();
extern int u_main_wc();

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
    if (runflags == RUNFLAGS_SMOKE_TEST) {
        assign_init_program("smoke-test");
    } else {
        assign_init_program("sh");
    }
}

void assign_init_program(char const* prog) {
    void* sp = allocate_page();
    if (!sp) {
        // TODO: panic
        return;
    }
    void *ksp = allocate_page();
    if (!ksp) {
        // TODO: panic
        return;
    }
    user_program_t *program = find_user_program(prog);
    process_t* p0 = &proc_table.procs[0];
    acquire(&p0->lock);
    init_proc(p0);

    // extra initialization on top of what init_proc does for us:
    p0->pid = alloc_pid();
    p0->trap.pc = (regsize_t)program->entry_point;
    p0->name = program->name;
    p0->stack_page = sp;
    p0->kstack_page = ksp;
    p0->trap.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);
    p0->ctx.regs[REG_SP] = (regsize_t)ksp + PAGE_SIZE;
    release(&p0->lock);
}

user_program_t* find_user_program(char const *name) {
    for (int i = 0; i < MAX_USERLAND_PROGS; i++) {
        if (userland_programs[i].name && !strncmp(name, userland_programs[i].name, 16)) {
            return &userland_programs[i];
        }
    }
    return 0;
}
