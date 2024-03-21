#include "drivers/drivers.h"
#include "drivers/uart/uart.h"
#include "fdt.h"
#include "kernel.h"
#include "pagealloc.h"
#include "pipe.h"
#include "plic.h"
#include "pmp.h"
#include "proc.h"
#include "riscv.h"
#include "runflags.h"
#include "spinlock.h"
#include "sys.h"
#include "timer.h"

#ifdef CONFIG_LCD_ENABLED
#include "drivers/hd44780/hd44780.h"
#endif

spinlock init_lock = 0;
int user_stack_size = 0;

void kinit(regsize_t hartid, uintptr_t fdt_header_addr) {
    acquire(&init_lock);
    unsigned int cpu_id = hartid;
    if (cpu_id != BOOT_HART_ID) {
        release(&init_lock);
        // TODO: support multi-core
        hard_park_hart();
    }
    plic_init();
    drivers_init();
#ifdef CONFIG_LCD_ENABLED
    lcd_init();
#endif
    kprintf("kinit: cpu %d\n", cpu_id);
    fdt_init(fdt_header_addr);
    kprintf("bootargs: %s\n", fdt_get_bootargs());
    init_trap_vector(hartid);
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
    user_stack_size = (runflags == RUNFLAGS_TINY_STACK) ? 512 : PAGE_SIZE;
    int running_tests = runflags & RUNFLAGS_TESTS;
    init_paged_memory(paged_mem_end);
    if (!running_tests) {
        do_page_report(paged_mem_end);
    }
    fs_init();
    init_process_table(runflags, hartid);
    init_pipes();
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
void init_trap_vector(regsize_t hartid) {
    trap_frame.regs[REG_TP] = hartid;
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
// run. The scheduler will also populate trap_frame with the context of the
// target user process. ret_to_user will restore the registers from it and
// switch back to user mode.
void kernel_timer_tick(regsize_t sp) {
    disable_interrupts();
#if !MIXED_MODE_TIMER
    // with MIXED_MODE_TIMER it's advanced in mtimertrap, otherwise we do that here:
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
#endif
    sched();
    enable_interrupts();
    regsize_t satp = 0;
    if (cpu.proc != 0) {
        patch_proc_sp(cpu.proc, sp);
        satp = cpu.proc->usatp;
    }
    ret_to_user(satp);
}

// kernel_plic_handler is the C entry point for PLIC interrupt handling.
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

void panic(char const *message) {
    disable_interrupts();
    uart_prints("panic: ");
    uart_prints(message);
    uart_prints("\n");
    hard_park_hart();
}
