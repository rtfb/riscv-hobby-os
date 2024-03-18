#ifndef _PROGRAMS_H_
#define _PROGRAMS_H_

#define MAX_USERLAND_PROGS 32

typedef struct user_program_s {
    void *entry_point;
    char const *name;
} user_program_t;

// defined in proc_test.c:
extern user_program_t userland_programs[MAX_USERLAND_PROGS];

// defined in proc_test.c:
user_program_t* find_user_program(char const *name);
void assign_init_program(char const* prog, char const *test_script);

// defined in userland.c:
extern int u_main_shell();
extern int u_main_hello();
extern int u_main_sysinfo();
extern int u_main_fmt();
extern int u_main_hanger();
extern int u_main_ps();
extern int u_main_cat();
extern int u_main_coma();
extern int u_main_wc();
extern int u_main_gpio();
extern int u_main_iter();
extern int u_main_test_printf();
extern int u_main_fibd();
extern int u_main_fib();
extern int u_main_wait();
extern int u_main_ls();
extern int u_main_clock();

#endif // ifndef _PROGRAMS_H_
