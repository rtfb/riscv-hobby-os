#include "mmreg.h"
#include "riscv.h"
#include "timer.h"

void init_timer() {
    machine_init_timer();
}

void cause_timer_interrupt_now() {
#if HAS_S_MODE
    csr_sip_set_flags(SIP_SSIP);
#else
    set_timer_after(0);
#endif
}
