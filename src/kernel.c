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
#include "riscv.h"
#include "pmp.h"
#include "timer.h"

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
    init_trap_vector();
    void* paged_mem_end = init_pmp();
    init_timer(); // must go after init_trap_vector because it might rewrite mtvec/mscratch
#if BOOT_MODE_M && HAS_S_MODE
    // Switch to S-Mode if possible. On machines where this will get executed,
    // it will return like a regular function and will continue execution of
    // kinit() with the privilege mode switched to S. On other machines, this
    // will not be called and kinit() will just keep executing.
    set_supervisor_mode();
#endif
    char const* str = "foo"; // this is a random string to test out %s in kprintf()
    void *p = (void*)0xabcdf10a; // this is a random hex to test out %p in kprintf()
    kprintf("kprintf test: str=%s, ptr=%p, pos int=%d, neg int=%d\n",
        str, p, 1337, MAX_NEG_INT);
    uint32_t runflags = parse_runflags();
    init_paged_memory(paged_mem_end, 0);
    int running_tests = runflags & RUNFLAGS_TESTS;
    if (running_tests) {
        // do_page_report();
    }
    init_process_table(runflags, hartid);
    init_pipes();
    fs_init();
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
#if HAS_S_MODE
    set_sscratch_csr(&trap_frame);
    set_stvec_csr(&trap_vector);
#else
    set_mscratch_csr(&trap_frame);
    set_mtvec_csr(&trap_vector);
#endif
}

// kernel_timer_tick will be called from timer to give kernel time to do its
// housekeeping as well as run the scheduler to pick the next user process to
// run.
regsize_t kernel_timer_tick() {
    disable_interrupts();
#if !MIXED_MODE_TIMER
    // with MIXED_MODE_TIMER it's advanced in mtimertrap, otherwise we do that here:
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
#endif
    sched();
    enable_interrupts();
    // return proc->usatp if possible so that we have it in a0 in k_interrupt_timer
    if (cpu.proc != 0) {
        return cpu.proc->usatp;
    }
    return 0;
}

// kernel_plic_handler...
void kernel_plic_handler() {
    disable_interrupts();
    plic_dispatch_interrupts();
    enable_interrupts();
    regsize_t satp = 0;
    if (cpu.proc != 0) {
        satp = cpu.proc->usatp;
    }
    ret_to_user(satp);
}

void disable_interrupts() {
    clear_status_interrupt_enable();
    set_ie_csr(0);
}

void enable_interrupts() {
    set_status_interrupt_pending();
    set_interrupt_enable_bits();
#if MIXED_MODE_TIMER
    csr_sip_clear_flags(SIP_SSIP);
#endif
}
