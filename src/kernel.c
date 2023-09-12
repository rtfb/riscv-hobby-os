#include "sys.h"
#include "kernel.h"
#include "spinlock.h"
#include "proc.h"
#include "fdt.h"
#include "pagealloc.h"
#include "drivers/uart/uart.h"
#include "drivers/hd44780/hd44780.h"
#include "pipe.h"
#include "runflags.h"
#include "drivers/drivers.h"
#include "plic.h"

spinlock init_lock = 0;

// #define OX64_UART0_BASE        0x2000a000
#define OX64_UART0_BASE        UART_BASE

#define OX64_UART_REG_UTX_CONFIG   0x00
#define OX64_UART_REG_INT_MASK     0x24
#define OX64_UART_REG_INT_EN       0x2c

#define OX64_UART_TX_ENABLE        (1 << 0)
#define OX64_UART_ENABLE_FREERUN   (1 << 2)

#define OX64_UART_TX_BREAK_BIT_CNT (4 << 10)
#define OX64_UART_TX_DATA_BIT_CNT  (7 << 8)

void uart_init_freerun() {
    *(uint32_t*)(OX64_UART0_BASE + OX64_UART_REG_UTX_CONFIG) = (
          OX64_UART_TX_BREAK_BIT_CNT
        | OX64_UART_TX_DATA_BIT_CNT
        | OX64_UART_ENABLE_FREERUN
        | OX64_UART_TX_ENABLE
    );
    *(uint32_t*)(OX64_UART0_BASE + OX64_UART_REG_INT_MASK) = ~0;
}

uint32_t get_uart_utx_config() {
    return *(uint32_t*)(OX64_UART0_BASE + OX64_UART_REG_UTX_CONFIG);
}

uint32_t get_uart_int_en() {
    return *(uint32_t*)(OX64_UART0_BASE + OX64_UART_REG_INT_EN);
}

void kinit(uintptr_t fdt_header_addr) {
    kprintf("k! int_en: %d\n", get_uart_int_en());
    uart_init_freerun();
    // kprintf("k!\n");
    acquire(&init_lock);
    unsigned int cpu_id = 0; // get_mhartid();
    if (cpu_id > 0) {
        release(&init_lock);
        // TODO: support multi-core
        park_hart();
    }
    plic_init();
    drivers_init();
    // lcd_init();
    // kprintf("kinit: cpu %d\n", cpu_id);
    kprintf("let's try printing a longish fixed string via UART\n");
    // fdt_init(fdt_header_addr);
    kprintf("bootargs: %s\n", fdt_get_bootargs());
    uint32_t runflags = parse_runflags();
    init_trap_vector();
    void* paged_mem_end = init_pmp();
    char const* str = "foo"; // this is a random string to test out %s in kprintf()
    void *p = (void*)0xf10a; // this is a random hex to test out %p in kprintf()
    kprintf("kprintf test several params: %s, %p, %d\n", str, p, cpu_id);
    int running_tests = runflags & RUNFLAGS_TESTS;
    init_paged_memory(paged_mem_end, !running_tests);
    init_process_table(runflags);
    init_global_trap_frame();
    init_pipes();
    fs_init();
    uint32_t *ctl_reg = (uint32_t*)OX64_TCCR_REG;
    uint32_t reg_val = *ctl_reg;
    kprintf("initial TCCR value=0x%x\n", reg_val);
    timer_init();
    reg_val = *ctl_reg;
    kprintf("TCCR value after init=0x%x\n", reg_val);
    uint64_t t0 = time_get_now();
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
    release(&init_lock);
    kprintf("scheduler()...\n");
    uint64_t t1 = time_get_now();
    kprintf("t0=%d\n", t0);
    kprintf("t1=%d\n", t1);
    // uint64_t *stimecmp = (uint64_t*)STIMECMP;
    // kprintf("stimecmp=%d\n", *stimecmp);
    // enable_interrupts();
    // uint64_t sstatus = get_mstatus();
    // kprintf("sstatusH=0x%x\n", sstatus >> 32);
    // kprintf("sstatusL=0x%x\n", sstatus & 0xffffffff);
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
    kprintf("T");
    set_timer_after(KERNEL_SCHEDULER_TICK_TIME);
    sched();
    enable_interrupts();
}

// kernel_plic_handler...
void kernel_plic_handler() {
    disable_interrupts();
    kprintf("plic\n");
    plic_dispatch_interrupts();
    enable_interrupts();
}

void set_mie(uint64_t value) {
    asm volatile (
        "csrs   sie, %0;"   // set mie to the requested value
        :                   // no output
        : "r"(value)        // input in value
    );
}

void disable_interrupts() {
    uint64_t mstatus = get_mstatus();
    // mstatus &= ~(1 << 3);
    mstatus &= ~(1 << 1);
    set_mstatus(mstatus);
    set_mie(0);
}

void enable_interrupts() {
    // set mstatus.MPIE (Machine Pending Interrupt Enable) bit to 1:
    uint64_t mstatus = get_mstatus();
    // mstatus |= (1 << MSTATUS_MPIE_BIT);
    mstatus |= (1 << MSTATUS_SPIE_BIT);
    set_mstatus(mstatus);

    // set the mie.MTIE (Machine Timer Interrupt Enable) bit to 1:
    // unsigned int mie = (1 << MIE_MTIE_BIT) | (1 << MIE_MEIE_BIT);
    uint64_t mie = (1 << MIE_STIE_BIT) | (1 << MIE_SEIE_BIT);
    set_mie(mie);
}

void set_mtvec(void *ptr) {
    asm volatile (
        "csrw   stvec, %0;" // set mtvec to the requested value
        :                   // no output
        : "r"(ptr)          // input in ptr
    );
}
