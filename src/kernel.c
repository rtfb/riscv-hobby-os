#include "sys.h"
#include "kernel.h"
#include "spinlock.h"
#include "proc.h"
#include "fdt.h"

spinlock init_lock = 0;

void kinit(uintptr_t fdt_header_addr) {
    acquire(&init_lock);
    unsigned int cpu_id = get_mhartid();
    if (cpu_id > 0) {
        kprints("cpu parked: ");
        kprintul(cpu_id);
        release(&init_lock);
        // TODO: support multi-core
        park_hart();
    }
    kprints("kinit: cpu ");
    kprintul(cpu_id);
    fdt_init(fdt_header_addr);
    kprints("bootargs: ");
    kprints(fdt_get_bootargs());
    kprints("\n");
    init_trap_vector();
    init_pmp();
    void *p = (void*)0xf10a; // this is a random hex to test out kprintp()
    kprintp(p);
    init_process_table();
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
    unsigned long addr = (unsigned long)&trap_vector;
    set_mtvec((void*)(addr | TRAP_VECTORED));
}

// kernel_timer_tick will be called from timer to give kernel time to do its
// housekeeping as well as run the scheduler to pick the next user process to
// run.
void kernel_timer_tick() {
    disable_interrupts();
    kprints("K");
    void* userland_pc = (void*)get_mepc();
    if (userland_pc && curr_proc >= 0) {
        proc_table[curr_proc].pc = userland_pc;
    }
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
    schedule_user_process();
    enable_interrupts();
}

void set_timer_after(uint64_t delta) {
    unsigned int hart_id = get_mhartid();
    uint64_t *mtime = (uint64_t*)MTIME;
    uint64_t *mtimecmp = (uint64_t*)(MTIMECMP_BASE) + 8*hart_id;
    uint64_t now = *mtime;
    *mtimecmp = now + delta;
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

void kprintp(void* p) {
    unsigned long pp = (unsigned long)p;
    kprintul(pp);
}

void kprintul(unsigned long i) {
    static char buf[64];
    int j = sizeof(i)*2 + 1;
    buf[j] = '\0';
    j--;
    buf[j] = '\n';
    j--;
    while (j >= 0) {
        char lowest_4_bits = i & 0xf;
        if (lowest_4_bits < 10) {
            buf[j] = '0' + lowest_4_bits;
        } else {
            buf[j] = 'a' + lowest_4_bits - 10;
        }
        j--;
        i >>= 4;
    }
    kprints(buf);
}
