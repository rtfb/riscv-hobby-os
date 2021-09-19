#ifndef _PROC_H_
#define _PROC_H_

// TODO: move other process-related stuff here

#define MAX_PROCS 2

typedef struct process_s {
    int pid;
    void *pc;
} process_t;

extern process_t proc_table[MAX_PROCS];
extern int num_procs;

// init_test_processes initializes the process table with a set of userland
// processes that will get executed by default. Kind of like what an initrd
// would do, but a poor man's version until we can do better.
void init_test_processes();

#endif // ifndef _PROC_H_
