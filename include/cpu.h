#ifndef _CPU_H_
#define _CPU_H_

#include "riscv.h"

// REG_* constants are indexes into trap_frame_t.regs and context_t.regs. (Add here as needed)
#define REG_RA 0
#define REG_SP 1
#define REG_TP 3
#define REG_FP 7
#define REG_A0 9
#define REG_A1 10
#define REG_A2 11
#define REG_A3 12
#define REG_A7 16

typedef struct context_s {
    regsize_t regs[14]; // ra, sp, s0..s11
} context_t;

typedef struct cpu_s {
    struct process_s *proc; // the process running on this cpu, or null
    context_t context;      // swtch() here to enter scheduler()
} cpu_t;

// An array of per-cpu state structs. We still actually run single-core, so only
// one of them will be actually used for now.
//
// defined in cpu.c
extern cpu_t cpus[NUM_HARTS];

// thiscpu returns a pointer one of cpus (see above). The index is assumed to be
// in the tp register.
cpu_t *thiscpu();

void init_cpus();

#endif // ifndef _CPU_H_
