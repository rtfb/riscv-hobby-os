#include "timer.h"

uint64_t time_get_now() {
    register uint64_t a0 asm ("a0");
    asm volatile (
        "csrr a0, time"
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void set_timer_after(uint64_t delta) {
    // TODO: implement
}
