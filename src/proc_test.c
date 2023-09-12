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
extern int u_main_gpio();

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
    // keep this last, it's a sentinel:
    (user_program_t){
        .entry_point = 0,
        .name = 0,
    },
};

void init_test_processes(uint32_t runflags) {
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
    userland_programs[7].entry_point = &u_main_ps;
    userland_programs[7].name = "ps";
    userland_programs[8].entry_point = &u_main_cat;
    userland_programs[8].name = "cat";
    userland_programs[9].entry_point = &u_main_coma;
    userland_programs[9].name = "coma";
    userland_programs[10].entry_point = &u_main_pipe;
    userland_programs[10].name = "pp";
    userland_programs[11].entry_point = &u_main_pipe2;
    userland_programs[11].name = "pp2";
    userland_programs[12].entry_point = &u_main_wc;
    userland_programs[12].name = "wc";
    userland_programs[13].entry_point = &u_main_gpio;
    userland_programs[13].name = "gpio";
    userland_programs[14].entry_point = 0;
    userland_programs[14].name = 0;
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
    kprintf("find_user_program('%s')\n", prog);
    user_program_t *program = find_user_program(prog);
    if (!program) {
        kprintf("!program\n");
    }
    process_t* p0 = alloc_process(sp, ksp);
    if (!p0) {
        kprintf("!p0\n");
    }
    acquire(&p0->lock);

    // extra initialization on top of what init_proc does for us:
    p0->pid = alloc_pid();
    p0->trap.pc = (regsize_t)program->entry_point;
    p0->name = program->name;
    p0->trap.regs[REG_SP] = (regsize_t)(sp + PAGE_SIZE);
    p0->ctx.regs[REG_SP] = (regsize_t)ksp + PAGE_SIZE;
    release(&p0->lock);
}

user_program_t* find_user_program(char const *name) {
    for (int i = 0; i < MAX_USERLAND_PROGS; i++) {
        user_program_t *p = &userland_programs[i];
        if (!p->name && !p->entry_point) {
            break;
        }
        int cmp = strncmp(name, p->name, 16);
        // kprintf("find_user_program[%d]: '%s', cmp=%d\n", i, p->name, cmp);
        kprintf("find_user_program[%d]: cmp=%d\n", i, cmp);
        if (p->name && !strncmp(name, p->name, 16)) {
            return p;
        }
    }
    return 0;
}
