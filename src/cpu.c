#include "cpu.h"
#include "mem.h"
#include "pmp.h"

cpu_t cpus[NUM_HARTS];

// defined in kernel.ld:
extern void* RAM_START;

void init_cpus() {
    memset(&cpus, sizeof(cpus), 0);
    for (int i = 0; i < NUM_HARTS; i++) {
        cpus[i].context.regs[REG_SP] = (regsize_t)(&RAM_START) + i*512;
    }
}
