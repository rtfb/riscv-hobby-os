#include "mmreg.h"
#include "riscv.h"
#include "timer.h"

#define STIMECMP_LO   0xd000
#define STIMECMP_HI   0xd004

void machine_init_timer() {
    // nothing to be done on this machine
}

uint64_t time_get_now() {
    register uint64_t a0 __asm__ ("a0");
    __asm__ __volatile__ (
        "csrr a0, time"
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void set_timer_after(uint64_t delta) {
    uint64_t now = time_get_now();
    uint64_t future = now + delta;

    // write the uint64_t value in two separate 32-bit writes:
    uint32_t future_lo = future & 0xffffffff;
    uint32_t future_hi = future >> 32;
    write32(CLINT0_BASE_ADDRESS + STIMECMP_LO, future_lo);
    write32(CLINT0_BASE_ADDRESS + STIMECMP_HI, future_hi);
}
