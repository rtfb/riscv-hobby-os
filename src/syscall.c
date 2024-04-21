#include "cpu.h"
#include "errno.h"
#include "kernel.h"
#include "proc.h"
#include "vm.h"

void syscall(regsize_t kernel_sp) {
    disable_interrupts();
    int nr = trap_frame.regs[REG_A7];
    process_t *proc = myproc();
    *proc->perrno = 0; // clear errno
    trap_frame.pc += 4; // step over the ecall instruction that brought us here
    regsize_t user_sp = (regsize_t)va2pa(proc->upagetable, (void*)trap_frame.regs[REG_SP]);
    if (user_sp < (regsize_t)proc->stack_page) {
        kprintf("STACK OVERFLOW in userland before pid:syscall %d:%d\n", proc->pid, nr);
        trap_frame.regs[REG_A0] = -1;
        *proc->perrno = EFAULT;
        // TODO: kill proc
        return;
    }
    if (nr >= 0 && nr <= SYSCALL_VECTOR_LEN && syscall_vector[nr] != 0) {
        int32_t (*funcPtr)(void) = syscall_vector[nr];
        trap_frame.regs[REG_A0] = (*funcPtr)();
    } else {
        kprintf("BAD pid:syscall %d:%d\n", proc->pid, nr);
        trap_frame.regs[REG_A0] = -1;
        *proc->perrno = ENOSYS;
    }
    if (*proc->magic != PROC_MAGIC_STACK_SENTINEL) {
        kprintf("STACK OVERFLOW in kernel pid:syscall %d:%d (magic=0x%x)\n",
            proc->pid, nr, *proc->magic);
        trap_frame.regs[REG_A0] = -1;
        *proc->perrno = EFAULT;
        panic("kernel stack overflow");
        return;
    }
    enable_interrupts();

    // we might have gotten here from an interrupt that occurred in M-/S-mode,
    // so ensure we will go back to U-mode, not back to one of the privileged
    // ones:
    set_user_mode();
    patch_proc_sp(proc, kernel_sp);
    ret_to_user(proc->usatp);
}
