#include "sys.h"
#include "kernel.h"
#include "spinlock.h"
#include "proc.h"
#include "fdt.h"
#include "pagealloc.h"
#include "drivers/uart/uart.h"
#include "pipe.h"
#include "runflags.h"
#include "drivers/drivers.h"
#include "plic.h"

#ifdef CONFIG_LCD_ENABLED
#include "drivers/hd44780/hd44780.h"
#endif

spinlock init_lock = 0;

void kinit(regsize_t hartid, uintptr_t fdt_header_addr) {
    acquire(&init_lock);
    unsigned int cpu_id = hartid;
    if (cpu_id > 0) {
        release(&init_lock);
        // TODO: support multi-core
        park_hart();
    }
    plic_init();
    drivers_init();
#ifdef CONFIG_LCD_ENABLED
    lcd_init();
#endif
    kprintf("kinit: cpu %d\n", cpu_id);
    fdt_init(fdt_header_addr);
    kprintf("bootargs: %s\n", fdt_get_bootargs());
    uint32_t runflags = parse_runflags();
    init_trap_vector();
    void* paged_mem_end = init_pmp();
    char const* str = "foo"; // this is a random string to test out %s in kprintf()
    void *p = (void*)0xf10a; // this is a random hex to test out %p in kprintf()
    kprintf("kprintf test several params: %s, %p, %d\n", str, p, cpu_id);
    int running_tests = runflags & RUNFLAGS_TESTS;
    init_paged_memory(paged_mem_end, !running_tests);
    init_process_table(runflags, hartid);
    init_global_trap_frame();
    init_pipes();
    fs_init();
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
    release(&init_lock);
    scheduler(); // done init'ing, now run the scheduler, forever
}

// 3.1.12 Machine Trap-Vector Base-Address Register (mtvec)
// > When MODE=Vectored, all synchronous exceptions into machine mode cause
// > the pc to be set to the address in the BASE field, whereas interrupts
// > cause the pc to be set to the address in the BASE field plus four times
// > the interrupt cause number. When vectored interrupts are enabled,
// > interrupt cause 0, which corresponds to user-mode software interrupts, are
// > vectored to the same location as synchronous > exceptions. This ambiguity
// > does not arise in practice, since user-mode software interrupts are either
// > disabled or delegated to a less-privileged mode.
//
// init_trap_vector initializes the exception, interrupt & syscall trap vector
// in the mtvec register.
//
// 3.1.12 Machine Trap-Vector Base-Address Register (mtvec),
// Table 3.5: Encoding of mtvec MODE field.
// All exceptions set pc to BASE.
// Exceptions set pc to BASE, interrupts set pc to BASE+4*cause.
//
// Trap vector mode is encoded in 2 bits: Direct = 0b00, Vectored = 0b01
// and is stored in 0:1 bits of mtvec CSR (mtvec.mode)
void init_trap_vector() {
    extern void* trap_vector;  // defined in boot.s
    set_tvec_csr(&trap_vector);
}

// kernel_timer_tick will be called from timer to give kernel time to do its
// housekeeping as well as run the scheduler to pick the next user process to
// run.
void kernel_timer_tick() {
    disable_interrupts();
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
    sched();
    enable_interrupts();
}

// kernel_plic_handler...
void kernel_plic_handler() {
    disable_interrupts();
    plic_dispatch_interrupts();
    enable_interrupts();
}

void disable_interrupts() {
    unsigned int status_csr = get_status_csr();
    status_csr &= ~(1 << 3);
    set_status_csr(status_csr);
    set_ie_csr(0);
}

void enable_interrupts() {
    // set mstatus.MPIE (Machine Pending Interrupt Enable) bit to 1:
    unsigned int status_csr = get_status_csr();
    status_csr |= (1 << MSTATUS_MPIE_BIT);
    set_status_csr(status_csr);

    // set the mie.MTIE (Machine Timer Interrupt Enable) bit to 1:
    unsigned int mie = (1 << MIE_MTIE_BIT) | (1 << MIE_MEIE_BIT);
    set_ie_csr(mie);
}
