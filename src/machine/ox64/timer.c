#include "timer.h"
#include "sbi.h"

void init_timer() {
    // nothing to be done on this machine
}

uint64_t time_get_now() {
    register uint64_t a0 asm ("a0");
    asm volatile (
        "csrr a0, time"
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void set_timer_after(uint64_t delta) {
    uint64_t now = time_get_now();
    sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, now+delta, 0, 0, 0, 0, 0);
}
