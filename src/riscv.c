#include "kernel.h"
#include "plic.h"

unsigned int get_mhartid() {
    register unsigned int a0 asm ("a0");
    asm volatile (
        "csrr a0, mhartid"
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void* shift_right_addr(void* addr, int bits) {
    unsigned long iaddr = (unsigned long)addr;
    return (void*)(iaddr >> bits);
}

void set_pmpaddr0(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr0, %0;"  // set pmpaddr0 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpaddr1(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr1, %0;"  // set pmpaddr1 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpaddr2(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr2, %0;"  // set pmpaddr2 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpaddr3(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr3, %0;"  // set pmpaddr3 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpcfg0(unsigned long value) {
    asm volatile (
        "csrw   pmpcfg0, %0;"   // set pmpcfg0 to the requested value
        :                       // no output
        : "r"(value)            // input in value
    );
}

uint64_t get_mstatus() {
    register uint64_t a0 asm ("a0");
    asm volatile (
        "csrr a0, sstatus"
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void set_mstatus(uint64_t value) {
    asm volatile (
        "csrw sstatus, %0"
        :            // no output
        : "r"(value) // input in value
    );
}

void* get_mepc() {
    register void* a0 asm ("a0");
    asm volatile (
        "csrr a0, sepc"
        : "=r"(a0)   // output in a0
    );
    return a0;
}

void set_jump_address(void *func) {
    asm volatile (
        "csrw   sepc, %0;"  // set mepc to userland function
        :                   // no output
        : "r"(func)         // input in func
    );
}

// Privilege levels are encoded in 2 bits: User = 0b00, Supervisor = 0b01,
// Machine = 0b11 and stored in 11:12 bits of mstatus CSR (called mstatus.mpp)
void set_user_mode() {
    unsigned int mstatus = get_mstatus();
    mstatus &= MODE_MASK;   // zero out mode bits 11:12
    mstatus |= MODE_U;      // set them to user mode
    set_mstatus(mstatus);
}

void set_mscratch(void* ptr) {
    asm volatile (
        "csrw sscratch, %0"
        :            // no output
        : "r"(ptr)   // input in value
    );
}

#define TIMER_IRQ_NUM   0x07

void timer_init() {

	/* clock enable */
	unsigned int reg;

	reg = *(volatile unsigned int *)0x30007000;
	reg |= (1 << 10); // mm_xclk <--- xtal
	reg |= (3 << 4);  // mm_uart_clk <--- mm_xclk
	// reg &= ~(1 << 11); // mm_cpu_clk <--- xclk
	*(volatile unsigned int *)0x30007000 = reg;
	__asm volatile("fence.i" ::: "memory");



    uint32_t *ctl_reg = (uint32_t*)OX64_TCCR_REG;
    *ctl_reg = (0xa5 << 24) | (1 << 8) | (5 << 4) | (1 << 0); // timer2=f32k
    // uint32_t *ticr2_reg = (uint32_t*)OX64_TICR2_REG;
    // *ticr2_reg = 1;       // clear interrupt for comparator 0 of timer2
    uint32_t *tcmr_reg = (uint32_t*)OX64_TCMR_REG;
    *tcmr_reg = (1 << 1); // timer2 free-run mode
    uint32_t *tier2_reg = (uint32_t*)OX64_TIER2_REG;
    *tier2_reg = 1;       // enable interrupts for comparator 0 of timer2
    uint32_t *tcer_reg = (uint32_t*)OX64_TCER_REG;
    *tcer_reg = (1 << 1); // timer2 enable

    plic_enable_intr(TIMER_IRQ_NUM);
    plic_set_intr_priority(TIMER_IRQ_NUM, PLIC_MAX_PRIORITY);
    plic_set_threshold(PLIC_MAX_PRIORITY - 1);
}

struct sbiret {
	long error;
	long value;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5)
{
	struct sbiret ret;

	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
	register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
	register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
	register uintptr_t a7 asm ("a7") = (uintptr_t)(ext);
	asm volatile ("ecall"
		      : "+r" (a0), "+r" (a1)
		      : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		      : "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

#define SBI_EXT_TIME            0x54494D45
#define SBI_EXT_TIME_SET_TIMER  0

void set_timer_after(uint64_t delta) {
    // unsigned int hart_id = get_mhartid();
    // uint64_t *mtime = (uint64_t*)MTIME;
    // uint64_t *mtimecmp = (uint64_t*)(MTIMECMP_BASE) + 8*hart_id;
    // uint64_t now = *mtime;
    // *mtimecmp = now + delta;

    // uint32_t *mtime = (uint32_t*)OX64_TCR2_REG;
    // uint32_t *mtimecmp = (uint32_t*)OX64_TMR2_0;
    // uint32_t now = *mtime;
    // uint32_t d32 = delta & 0xffffffff;
    // *mtimecmp = now + d32;

    // uint64_t now = time_get_now();
    // uint64_t *stimecmp = (uint64_t*)STIMECMP;
    // *stimecmp = now + delta;

    // uint64_t now = time_get_now();
    // uint64_t *stimecmp = (uint64_t*)MTIMECMP_BASE;
    // *stimecmp = now + delta;

    uint64_t now = time_get_now();
    sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, now+delta, 0, 0, 0, 0, 0);
}

uint64_t time_get_now() {
    // uint64_t *mtime = (uint64_t*)MTIME;
    // return *mtime;

    // uint32_t *mtime = (uint32_t*)OX64_TCR2_REG;
    // uint64_t time = *mtime;
    // return time;

    register uint64_t a0 asm ("a0");
    asm volatile (
        "csrr a0, time"
        : "=r"(a0)   // output in a0
    );
    return a0;
}
