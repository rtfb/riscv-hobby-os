#include "sys.h"
#include "kernel.h"
#include "spinlock.h"
#include "proc.h"
#include "fdt.h"
#include "pagealloc.h"
#include "uart.h"

spinlock init_lock = 0;

void kinit(uintptr_t fdt_header_addr) {
    acquire(&init_lock);
    unsigned int cpu_id = get_mhartid();
    if (cpu_id > 0) {
        kprintf("cpu parked: %d\n", cpu_id);
        release(&init_lock);
        // TODO: support multi-core
        park_hart();
    }
    uart_init();
    kprintf("kinit: cpu %d\n", cpu_id);
    fdt_init(fdt_header_addr);
    kprintf("bootargs: %s\n", fdt_get_bootargs());
    init_trap_vector();
    void* paged_mem_end = init_pmp();
    char const* str = "foo"; // this is a random string to test out %s in kprintf()
    void *p = (void*)0xf10a; // this is a random hex to test out %p in kprintf()
    kprintf("kprintf test several params: %s, %p, %d\n", str, p, cpu_id);
    init_paged_memory(paged_mem_end);
    init_process_table();
    init_global_trap_frame();
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
    enable_interrupts();
    release(&init_lock);
    // after kinit() is done, halt this hart until the timer gets called, all
    // the remaining kernel ops will be orchestrated from the timer
    park_hart();
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
// and is stored in 0:1 bits of mtvect CSR (mtvec.mode)
void init_trap_vector() {
    extern void* trap_vector;  // defined in boot.s
    set_mtvec(&trap_vector);
}

// kernel_timer_tick will be called from timer to give kernel time to do its
// housekeeping as well as run the scheduler to pick the next user process to
// run.
void kernel_timer_tick() {
    disable_interrupts();
    regsize_t userland_pc = trap_frame.pc;
    acquire(&proc_table.lock);
    if (userland_pc && !proc_table.is_idle) {
        proc_table.procs[proc_table.curr_proc].context.pc = userland_pc;
    }
    release(&proc_table.lock);
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
    schedule_user_process();
    enable_interrupts();
}

void set_mie(unsigned int value) {
    asm volatile (
        "csrs   mie, %0;"   // set mie to the requested value
        :                   // no output
        : "r"(value)        // input in value
    );
}

void disable_interrupts() {
    set_mie(0);
}

void enable_interrupts() {
    // set mstatus.MIE (Machine Interrupt Enable) bit to 1:
    unsigned int mstatus = get_mstatus();
    mstatus |= 1 << 3;
    set_mstatus(mstatus);

    // set the mie.MTIE (Machine Timer Interrupt Enable) bit to 1:
    set_mie(1 << 7);
}

void set_mtvec(void *ptr) {
    asm volatile (
        "csrw   mtvec, %0;" // set mtvec to the requested value
        :                   // no output
        : "r"(ptr)          // input in ptr
    );
}
