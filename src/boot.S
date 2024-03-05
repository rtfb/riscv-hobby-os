#include "asm.h"
.equ HALT_ON_EXCEPTION, 1
.equ BOOT_HART_ID, 0
.equ RISCV_HEADER_VERSION, 0x2

.balign 4
.section .text

// This section between _start and _start_kernel is a hardcoded Linux kernel
// signature. Functionally, it does nothing, but immediately jumps to
// _start_kernel. It is only here to be found in the first few bytes of the
// binary by the bootloader, otherwise it refuses to recognize the binary and
// load it as kernel.
//
// More info about the header format here:
// https://www.kernel.org/doc/html/v5.8/riscv/boot-image-header.html
.globl _start
_start:
        j _start_kernel
        // reserved
        .word 0
        .balign 8
        .dword LINUX_IMAGE_HEADER_TEXT_OFFSET
        // Effective size of kernel image
        .dword _end - _start
        .dword 0
        .word RISCV_HEADER_VERSION
        .word 0
        .dword 0
        .ascii "RISCV\0\0\0"
        .balign 4
        .ascii "RSC\x05"
        .word 0

.globl _start_kernel
_start_kernel:
                                        // if not specified otherwise, the source for quotations is:
                                        //       The RISC-V Instruction Set Manual
                                        //       Volume II: Privileged Architecture
                                        //       Privileged Architecture Version 1.10

                                        // from: sifive-interrupt-cookbook-v1p2.pdf
                                        // 2.1.3 Early Boot: Setup mtvec Register
        csrwi   BOOT_REG_IE, 0          // > It is recommended to disable interrupts globally using mstatus.mie prior to changing mtvec.
                                        // > For sanity's sake we set up an early trap vector that just does nothing.
        la      t0, early_trap_vector
        csrw    BOOT_REG_TVEC, t0

#if BOOT_MODE_M && HAS_S_MODE
        csrwi   mideleg, 0              // disable trap delegation, all interrupts and exceptions will be handled in machine mode
        csrwi   medeleg, 0              // 3.1.8 Machine Trap Delegation Registers (medeleg and mideleg)
                                        // > In systems with S-mode, the medeleg and mideleg registers must exist, and
                                        // > setting a bit in medeleg or mideleg will delegate the corresponding trap,
                                        // > when occurring in S-mode or U-mode, to the S-mode trap handler. In systems
                                        // > without S-mode, the medeleg and mideleg registers should not exist.
#endif

#if BOOT_MODE_M
        csrr    a0, mhartid             // set a0 to mhartid to be compatible with boot in S-mode
#endif
                                        // setup stack pointer:
        la      t0, stack_top_addr      // set it at stack_top_addr for hart0,
        mv      t1, a0                  // at stack_top_addr-512 for hart1, etc.
        li      t2, 512
        mul     t1, t1, t2
        sub     t0, t0, t1
        mv      sp, t0

        // only allow mhartid==BOOT_HART_ID to jump to the init_segments code;
        // all other harts continue to run hart_sync_code, then unconditionally
        // jump to init.
        li      t0, BOOT_HART_ID
        mv      t1, a0
        beq     t0, t1, init_segments

hart_sync_code:
        // Only non-boot harts run this code:
        // Step 1: every non-boot hart writes 1 to MSIP register:
        mv      t2, a0        // t2 = hart ID
        li      t0, 1
        li      t1, CLINT0_BASE_ADDRESS  // t1 = Core-Local Interrupt Controller base address
        slli    t2, t2, 2     // t2 = hartID*4
        add     t1, t1, t2    // t1 = address of MSIP mem-mapped register
        sw      t0, (t1)      // MSIP is 32 bits wide on all XLENs, so use sw
        fence   w,rw

        // Step 2: now busy-wait until the boot hart will clear that bit. When
        // it does, we're safe to proceed:
1:
        lw      t0, (t1)
        bnez    t0, 1b

        // now that we've gone through hart sync dance, continue to higher level
        // init code where each hart is allowed to run:
        j       init

init_segments:

        // This section of code between init_segments and synchronize_harts will
        // only be executed by a hart with id==BOOT_HART_ID. This ensures that
        // the .bss and .data segments (and other, as may become necessary) are
        // initialized once only.

        la      t0, bss_start
        la      t1, bss_end
        bgeu    t0, t1, synchronize_harts
clean_bss_loop:
        sw      zero, (t0)
        addi    t0, t0, 4
        bltu    t0, t1, clean_bss_loop

        // Now that the memory segments are init'ed, the boot hart will
        // synchronize all other harts: loop from 1 to NUM_HARTS, and for all of
        // them do the following: read its MSIP register, wait until they all
        // are 1. When all 1s are read from all harts, write 0s back to all of
        // them, allowing them all to proceed.
synchronize_harts:
        li      t0, 0                   // t0 = hart ID
        li      t1, CLINT0_BASE_ADDRESS // t1 = Core-Local Interrupt Controller base address
        li      t5, NUM_HARTS           // t5 = NUM_HARTS
        li      t6, BOOT_HART_ID        // t6 = boot hart ID, so that we can skip over it

        // if we're running on a single-harted machine, there's nothing to sync,
        // so jump over:
        li      t4, 1
        beq     t4, t5, init

        // Read loop: wait for MSIP=1 from each hart
per_hart_read_loop:
        beq     t0, t6, skip_boot_hart
        slli    t2, t0, 2               // t2 = hartID*4
        add     t3, t1, t2              // t3 = address of MSIP mem-mapped register
        lw      t4, (t3)                // MSIP is 32 bits wide on all XLENs, so use lw
        beq     t4, zero, per_hart_read_loop

skip_boot_hart:
        addi    t0, t0, 1
        blt     t0, t5, per_hart_read_loop    // jump if hartID < NUM_HARTS

        // Write loop: write MSIP=0 for each hart
        li      t0, 0                   // t0 = hart ID
per_hart_write_loop:
        beq     t0, t6, skip_boot_hart2
        slli    t2, t0, 2               // t2 = hartID*4
        add     t3, t1, t2              // t3 = address of MSIP mem-mapped register
        sw      zero, (t3)

skip_boot_hart2:
        addi    t0, t0, 1
        blt     t0, t5, per_hart_write_loop  // jump if hartID < NUM_HARTS

init:

                                        // @TODO: check if user mode is supported
                                        // see: https://github.com/riscv/riscv-tests/blob/master/isa/rv64si/csr.S
                                        // OR alternatively use the trick of setting exception handler trailing the operation
                                        // see: https://github.com/riscv/riscv-test-env/blob/master/p/riscv_test.h
                                        // >      la t0, 1f
                                        // >      csrw mtvec, t0
                                        // >      ... code that might fail due to lack of support in CPU
                                        // > 1:

        call    kinit                   // kinit() will set up the kernel for
                                        // further operations and never return.

        j exception                     // jump to exception here, just in case


// Trap Tables & Dispatchers #################################################
//
                                        // from: sifive-interrupt-cookbook-v1p2.pdf
                                        // 2.1.3 Early Boot: Setup mtvec Register
                                        // > For sanity's sake we set up an early trap vector that just does nothing.
                                        // > If you end up here then there's a bug in the early boot code somewhere.
early_trap_vector:
1:      csrr    t0, REG_CAUSE
        csrr    t1, REG_EPC
        csrr    t2, REG_TVAL
        j       1b                      // loop indefinitely

.globl mtimertrap
.balign 64
mtimertrap:
        // t0 = &timer_trap
        csrrw   t0, mscratch, t0

        // backup t1-3
        OP_STOR t1, 0*REGSZ(t0)
        OP_STOR t2, 1*REGSZ(t0)
        OP_STOR t3, 2*REGSZ(t0)

        // obtain MTIMECMP_BASE and timer interval and schedule the next tick
        OP_LOAD t1, 3*REGSZ(t0)
        OP_LOAD t2, 4*REGSZ(t0)
        OP_LOAD t3, 0(t1)
        add     t3, t3, t2
        OP_STOR t3, 0(t1)

        // restore t1-3
        OP_LOAD t1, 0*REGSZ(t0)
        OP_LOAD t2, 1*REGSZ(t0)
        OP_LOAD t3, 2*REGSZ(t0)

        // set mip.SSIP bit to cause mret to trap into trap_vector in S-Mode
        csrwi   mip, 0x2

        // restore t0 from mscratch
        csrrw   t0, mscratch, t0

        // mret into S-Mode
        mret

                                        // 3.1.12 Machine Trap-Vector Base-Address Register (mtvec)
                                        // > When MODE=Vectored, all synchronous exceptions into machine mode cause
                                        // > the pc to be set to the address in the BASE field, whereas interrupts cause
                                        // > the pc to be set to the address in the BASE field plus four times the interrupt cause number.
                                        // >
                                        // > When vectored interrupts are enabled, interrupt cause 0, which corresponds to user-mode
                                        // > software interrupts, are vectored to the same location as synchronous exceptions.
                                        // > This ambiguity does not arise in practice, since user-mode software interrupts are
                                        // > either disabled or delegated to a less-privileged mode.


                                        // from: https://stackoverflow.com/questions/58726942/risc-v-exceptions-vs-interrupts
                                        // @TODO: figure out correct section as stackoverflow answer does not match the v1.10 document anymore :(
                                        // > Section 3.1.15 of the privileged RISC-V specification:
                                        // > when an interrupt is raised MEPC points to the first uncompleted instruction AND
                                        // > when an exception is raised MEPC points to the instruction that raised an exception.
                                        // >
                                        // > Section 1.6 of the unprivileged RISC-V specification defines that:
                                        // > a) exceptions are raised by instructions and
                                        // > b) interrupts are raised by external events.
                                        // >
                                        // > When an (synchronous) exception is raised, the triggering instruction cannot be completed properly.
                                        // > Hence, there are two possibilities for the return address: either the instruction itself or
                                        // > the following instruction. Both solutions make sense. If it points to the instruction itself
                                        // > it is easier to determine the problem and react accordingly. If it points to the next instruction,
                                        // > the address need not be incremented when returning from the exception handler.
                                        // >
                                        // > (Asynchronous) interrupts are different, they break the stream of executed instructions of
                                        // > an independent thread. Therefore, there is only one reasonable solution for the return address:
                                        // > the first instruction that has not completed yet. Thus, when returning from the interrupt handler,
                                        // > the execution continues exactly where it was interrupted.

.globl trap_vector
.balign 64
trap_vector:                            // 3.1.20 Machine Cause Register (mcause), Table 3.6: Machine cause register (mcause) values after trap.
        // swap t6 and mscratch. t6 now points to trap_frame and the
        // actual value of t6 is saved in mscratch until we can restore it a bit
        // later:
        csrrw   t6, REG_SCRATCH, t6

        // save all user registers in trap_frame:
        OP_STOR  x1,  0*REGSZ(t6)
        OP_STOR  x2,  1*REGSZ(t6)
        OP_STOR  x3,  2*REGSZ(t6)
        OP_STOR  x4,  3*REGSZ(t6)
        OP_STOR  x5,  4*REGSZ(t6)
        OP_STOR  x6,  5*REGSZ(t6)
        OP_STOR  x7,  6*REGSZ(t6)
        OP_STOR  x8,  7*REGSZ(t6)
        OP_STOR  x9,  8*REGSZ(t6)
        OP_STOR x10,  9*REGSZ(t6)
        OP_STOR x11, 10*REGSZ(t6)
        OP_STOR x12, 11*REGSZ(t6)
        OP_STOR x13, 12*REGSZ(t6)
        OP_STOR x14, 13*REGSZ(t6)
        OP_STOR x15, 14*REGSZ(t6)
        OP_STOR x16, 15*REGSZ(t6)
        OP_STOR x17, 16*REGSZ(t6)
        OP_STOR x18, 17*REGSZ(t6)
        OP_STOR x19, 18*REGSZ(t6)
        OP_STOR x20, 19*REGSZ(t6)
        OP_STOR x21, 20*REGSZ(t6)
        OP_STOR x22, 21*REGSZ(t6)
        OP_STOR x23, 22*REGSZ(t6)
        OP_STOR x24, 23*REGSZ(t6)
        OP_STOR x25, 24*REGSZ(t6)
        OP_STOR x26, 25*REGSZ(t6)
        OP_STOR x27, 26*REGSZ(t6)
        OP_STOR x28, 27*REGSZ(t6)
        OP_STOR x29, 28*REGSZ(t6)
        OP_STOR x30, 29*REGSZ(t6)

        // x31 is the same as t6, so store it below with a bit of juggling:
        mv      t0, t6
        csrrw   t6, REG_SCRATCH, t6
        OP_STOR t6, 30*REGSZ(t0)

#if CONFIG_MMU
        la      t6, paged_memory
        OP_LOAD t6, (t6)
        csrw    satp, t6         // set satp to paged_memory.ksatp
        sfence.vma zero, zero
#endif

        csrr    t6, REG_EPC
        OP_STOR t6, 31*REGSZ(t0)

        // Restore sp from cpu.proc->ctx[REG_SP].
        la      t0, cpu
        OP_LOAD t0, 0*REGSZ(t0)  // t0 = cpu.proc
        // cpu.proc may be NULL, so we check for that and if so, jump to
        // set_global_stack. This can happen if the scheduler had nothing to
        // schedule and the CPU was idling.
        beq     x0, t0, set_global_stack
        OP_LOAD sp, 2*REGSZ(t0)  // (*process_t)[0] => lock
                                 // (*process_t)[1] => ctx[0] (RA)
                                 // (*process_t)[2] => ctx[1] (SP)
        j       stack_done

set_global_stack:
        // set the same stack location as we do during the boot time.
        la      t0, stack_top_addr         // set it at stack_top_addr for hart0,

        // XXX: change that when SMP gets implemented
        // csrr    t1, mhartid             // at stack_top_addr-512 for hart1, etc.
        mv      t1, zero

        li      t2, 512
        mul     t1, t1, t2
        sub     t0, t0, t1
        mv      sp, t0

stack_done:
        csrr    t0, REG_CAUSE
        bgez    t0, exception_dispatch

        slli    t0, t0, 2               // clears the top bit and multiplies the interrupt index by 4 at the same time
        la      t1, interrupt_vector
        add     t0, t1, t0
        jalr    t0

interrupt_vector:
.balign 4
        j exception_dispatch            //  0: user software interrupt OR _exception_ (See note in 3.1.12: Machine Trap-Vector Base-Address Register)
.balign 4
        j k_interrupt_timer             //  1: supervisor software interrupt
.balign 4
        j interrupt_noop                //  2: reserved
.balign 4
        j interrupt_noop                //  3: machine software interrupt
.balign 4
        j interrupt_timer               //  4: user timer interrupt
.balign 4
        j k_interrupt_timer             //  5: supervisor timer interrupt
.balign 4
        j interrupt_noop                //  6: reserved
.balign 4
        j k_interrupt_timer             //  7: machine timer interrupt
.balign 4
        j interrupt_noop                //  8: user external interrupt
.balign 4
        j k_interrupt_plic              //  9: supervisor external interrupt
.balign 4
        j interrupt_noop                // 10: reserved
.balign 4
        j k_interrupt_plic              // 11: machine external interrupt

exception_vector:                       // 3.1.20 Machine Cause Register (mcause), Table 3.6: Machine cause register (mcause) values after trap.
.balign 4
        j exception                     //  0: instruction address misaligned
.balign 4
        j exception                     //  1: instruction access fault
.balign 4
        j exception                     //  2: illegal instruction
.balign 4
        j exception                     //  3: breakpoint
.balign 4
        j exception                     //  4: load address misaligned
.balign 4
        j exception                     //  5: load access fault
.balign 4
        j exception                     //  6: store/AMO address misaligned
.balign 4
        j exception                     //  7: store/AMO access fault
.balign 4
        j syscall_dispatch              //  8: environment call from U-mode
.balign 4
        j syscall_dispatch              //  9: environment call from S-mode
.balign 4
        j exception                     // 10: reserved
.balign 4
        j syscall_dispatch              // 11: environment call from M-mode
.balign 4
        j exception                     // 12: instruction page fault
.balign 4
        j exception                     // 13: load page fault
.balign 4
        j exception                     // 14: reserved
.balign 4
        j exception                     // 15: store/AMO page fault
.balign 4
        j exception                     // 16: reserved
.balign 4
exception_vector_end:

exception_dispatch:
        la      t0, exception_vector
        csrr    t1, REG_CAUSE
        slli    t1, t1, 2
        add     t0, t0, t1              // jump -> exception_vector + mcause * 4
                                        // check out-of-bounds
        la      t1, exception_vector_end
        bgeu    t0, t1, exception_epilogue
        jr      t0

syscall_dispatch:
        mv      a0, sp
        call    syscall

exception_epilogue:
.if HALT_ON_EXCEPTION == 1
1:      j       1b
.endif
        li      a0, 0xbaadf00d     // TODO: figure out whether this codepath is ever used
        j       ret_to_user

interrupt_epilogue:
        OP_xRET


//## Exception, Interrupt & Syscall Handlers ###################################
//

exception:
                                        // no need to save: x0 - constant
                                        //                  t0, t1 - done in exception_dispatch
                                        //                  sp, gp, tp - not changed
                                        //                  s0..s7 - preserved inside functions
        OP_STOR x1,   1*REGSZ(sp)       // ra :: x1
        OP_STOR x7,   7*REGSZ(sp)       // t2 :: x7
        OP_STOR x10, 10*REGSZ(sp)       // a0 :: x10
        OP_STOR x11, 11*REGSZ(sp)
        OP_STOR x12, 12*REGSZ(sp)
        OP_STOR x13, 13*REGSZ(sp)
        OP_STOR x14, 14*REGSZ(sp)
        OP_STOR x15, 15*REGSZ(sp)
        OP_STOR x16, 16*REGSZ(sp)
        OP_STOR x17, 17*REGSZ(sp)       // a7 :: x17
        OP_STOR x28, 28*REGSZ(sp)       // t3 :: x28
        OP_STOR x29, 29*REGSZ(sp)
        OP_STOR x30, 30*REGSZ(sp)
        OP_STOR x31, 31*REGSZ(sp)       // t6 :: x31

        csrr    t0, REG_CAUSE
        csrr    t1, REG_EPC
        csrr    t2, REG_TVAL
        la      t3, user_payload
        la      t4, stack_top_addr
        addi    sp, sp, -5*REGSZ
        OP_STOR t0, 0*REGSZ(sp)
        OP_STOR t1, 1*REGSZ(sp)
        OP_STOR t2, 2*REGSZ(sp)
        OP_STOR t3, 3*REGSZ(sp)
        OP_STOR t4, 4*REGSZ(sp)
        la      a0, msg_exception
        mv      a1, sp
        call    kprintfvec
        addi    sp, sp, 5*REGSZ

        OP_LOAD x1,   1*REGSZ(sp)       // ra :: x1
        OP_LOAD x7,   7*REGSZ(sp)       // t2 :: x7
        OP_LOAD x10, 10*REGSZ(sp)       // a0 :: x10
        OP_LOAD x11, 11*REGSZ(sp)
        OP_LOAD x12, 12*REGSZ(sp)
        OP_LOAD x13, 13*REGSZ(sp)
        OP_LOAD x14, 14*REGSZ(sp)
        OP_LOAD x15, 15*REGSZ(sp)
        OP_LOAD x16, 16*REGSZ(sp)
        OP_LOAD x17, 17*REGSZ(sp)       // a7 :: x17
        OP_LOAD x28, 28*REGSZ(sp)       // t3 :: x28
        OP_LOAD x29, 29*REGSZ(sp)
        OP_LOAD x30, 30*REGSZ(sp)
        OP_LOAD x31, 31*REGSZ(sp)       // t6 :: x31
        j       exception_epilogue

interrupt_noop:
        j       interrupt_epilogue

interrupt_timer:
        j       interrupt_epilogue

.globl park_hart
park_hart:
loop:      wfi                  // parked hart will sleep waiting for interrupt
           j       loop

//## User payload = code + readonly data for U-mode ############################
//
.section .user_text                     // at least in QEMU 5.1 memory protection seems to work on 4KiB page boundaries,
                                        // so store user payload part in its own section, which is aligned on 4KiB to make
                                        // the further experiments more predictable
.globl user_payload
user_payload:

//## Data ######################################################################
//

.section .rodata
.globl rodata
rodata:
msg_exception:
        .string "Exception occurred, mcause:%p mepc:%p mtval:%p (user payload at:%p, stack top:%p).\n"

.section .data
.globl data
bss_zero_loop_lock:
        .word 0
