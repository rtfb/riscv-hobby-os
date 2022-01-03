#ifndef _PROC_H_
#define _PROC_H_

#include "riscv.h"

#define MAX_PROCS 2

typedef struct process_s {
    int pid;
    void *pc;
} process_t;

extern process_t proc_table[MAX_PROCS];
extern int num_procs;
extern int curr_proc;

// init_test_processes initializes the process table with a set of userland
// processes that will get executed by default. Kind of like what an initrd
// would do, but a poor man's version until we can do better.
void init_test_processes();
void init_process_table();
void schedule_user_process();

#endif // ifndef _PROC_H_
