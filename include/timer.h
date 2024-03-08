#ifndef _TIMER_H_
#define _TIMER_H_

#include "sys.h"

// These addresses are taken from the SiFive E31 core manual[1],
// Chapter 8: Core Local Interruptor (CLINT)
// [1] https://static.dev.sifive.com/E31-RISCVCoreIP.pdf
#define MTIME             0x200bff8
#define HART_OFFSET       8*BOOT_HART_ID
#define MTIMECMP_BASE     (0x2004000 + HART_OFFSET)

// Indicates that the timer is handled in M-Mode, the next timer tick is
// incremented in M-Mode as well, and for the rest of the timer functionality a
// software interrupt is retriggered into an S-Mode.
#define MIXED_MODE_TIMER  (BOOT_MODE_M && HAS_S_MODE)

#define KERNEL_SCHEDULER_TICK_TIME (ONE_SECOND)

typedef struct timer_trap_scratch_s {
    regsize_t   t1;
    regsize_t   t2;
    regsize_t   t3;
    void*       mtimercmp;
    regsize_t   interval;
} timer_trap_scratch_t;

// defined in timer.c
extern timer_trap_scratch_t timer_trap;

// defined in boot.S
extern void* mtimertrap;

void init_timer();
void set_timer_after(uint64_t delta);
uint64_t time_get_now();

#endif // ifndef _TIMER_H_
