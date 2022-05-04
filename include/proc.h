#ifndef _PROC_H_
#define _PROC_H_

#include "riscv.h"
#include "sys.h"

#define MAX_PROCS 2

// REG_* constants are indexes into trap_frame_t.regs. (Add here as needed)
#define REG_RA 0
#define REG_SP 1

typedef struct trap_frame_s {
    regsize_t regs[32];
} trap_frame_t;

typedef struct process_s {
    uint32_t pid;
    void *pc;
    trap_frame_t context;
} process_t;

extern process_t proc_table[MAX_PROCS];
extern int num_procs;
extern int curr_proc;

// trap_frame is the piece of memory to hold all user registers when we enter
// the trap. When the scheduler picks the new process to run, it will save
// trap_frame in the process_t of the old process and will populate trap_frame
// with the new process's context.
//
// When we're ready to return to userland, trap_frame is expected to already
// contain everything the userland needs to have. In case of a fresh process, it
// needs to have at least regs[REG_SP] populated.
//
// In between the traps mscratch will point here so that we can save user
// registers without trashing any.
extern trap_frame_t trap_frame;

// init_test_processes initializes the process table with a set of userland
// processes that will get executed by default. Kind of like what an initrd
// would do, but a poor man's version until we can do better.
void init_test_processes();
void init_process_table();
void schedule_user_process();

// init_global_trap_frame makes sure that mscratch contains a pointer to
// trap_frame before the first userland process gets scheduled.
void init_global_trap_frame();

#endif // ifndef _PROC_H_
