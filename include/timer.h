#ifndef _TIMER_H_
#define _TIMER_H_

#include "sys.h"

// These addresses are taken from the SiFive E31 core manual[1],
// Chapter 8: Core Local Interruptor (CLINT)
// [1] https://static.dev.sifive.com/E31-RISCVCoreIP.pdf
#define MTIME             0x200bff8
#define MTIMECMP_BASE     0x2004000

void set_timer_after(uint64_t delta);
uint64_t time_get_now();

#endif // ifndef _TIMER_H_
