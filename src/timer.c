#include "timer.h"
#include "riscv.h"

void set_timer_after(uint64_t delta) {
    unsigned int hart_id = get_hartid();
    uint64_t *mtime = (uint64_t*)MTIME;
    uint64_t *mtimecmp = (uint64_t*)(MTIMECMP_BASE) + 8*hart_id;
    uint64_t now = *mtime;
    *mtimecmp = now + delta;
}

uint64_t time_get_now() {
    uint64_t *mtime = (uint64_t*)MTIME;
    return *mtime;
}
