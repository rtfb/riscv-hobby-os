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
    uint32_t *stimecmp = (uint32_t*)(0x1400d000);
    uint64_t now = time_get_now();
    uint64_t future = now + delta;

    // write the uint64_t value in two separate 32-bit writes:
    uint32_t future_lo = future & 0xffffffff;
    uint32_t future_hi = future >> 32;
    *stimecmp = future_lo;
    uint32_t *stimecmphi = (uint32_t*)(0x1400d004);
    *stimecmphi = future_hi;
}
