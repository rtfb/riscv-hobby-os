#include "mmreg.h"
#include "riscv.h"
#include "timer.h"

timer_trap_scratch_t timer_trap;

void init_timer() {
    timer_trap.mtimercmp = (void*)MTIMECMP_BASE;
    timer_trap.interval = KERNEL_SCHEDULER_TICK_TIME;
#if BOOT_MODE_M && HAS_S_MODE
    set_mscratch_csr(&timer_trap);
    set_mtvec_csr(&mtimertrap);
#endif
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
}

void set_timer_after(uint64_t delta) {
    uint64_t now = time_get_now();
    uint64_t future = now + delta;
    write64(MTIMECMP_BASE, future);
}

uint64_t time_get_now() {
    return read64(MTIME);
}
