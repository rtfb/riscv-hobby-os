.include "src/machine-word.inc"
.equ STACK_PER_HART,    64 * REGBYTES
.equ HALT_ON_EXCEPTION, 0

.balign 4
.section .text
.globl _start
_start:
                                        # if not specified otherwise, the source is:
                                        #       The RISC-V Instruction Set Manual
                                        #       Volume II: Privileged Architecture
                                        #       Privileged Architecture Version 1.10

                                        # from: sifive-interrupt-cookbook-v1p2.pdf
                                        # 2.1.3 Early Boot: Setup mtvec Register
        csrwi   mie, 0                  # > It is recommended to disable interrupts globally using mstatus.mie prior to changing mtvec.
                                        # > For sanity's sake we set up an early trap vector that just does nothing.
        la      t0, early_trap_vector
        csrw    mtvec, t0

        csrwi   mideleg, 0              # disable trap delegation, all interrupts and exceptions will be handled in machine mode
        csrwi   medeleg, 0              # 3.1.13 Machine Trap Delegation Registers (medeleg and mideleg)
                                        # > In systems with all three privilege modes (M/S/U), setting a bit in medeleg or mideleg
                                        # > will delegate the corresponding trap in S-mode or U-mode to the S-mode trap handler.
                                        # > In systems with two privilege modes (M/U), setting a bit in medeleg or mideleg
                                        # > will delegate the corresponding trap in U-mode to the U-mode trap handler.

                                        # @TODO: support multi-core
        csrr    a0, mhartid             # read hardware thread id (`hart` stands for `hardware thread`)
        beqz    a0, single_core         # park all harts except the 1st one
1:      wfi                             # parked hart will sleep waiting for interrupt
        j       1b

single_core:                            # only the 1st hart past this point
        la      sp, stack_top           # setup stack pointer

        la      t0, bss_start
        la      t1, bss_end
        bgeu    t0, t1, init
clean_bss_loop:
        sw      zero, (t0)
        addi    t0, t0, 4
        bltu    t0, t1, clean_bss_loop
init:

                                        # @TODO: check if user mode is supported
                                        # see: https://github.com/riscv/riscv-tests/blob/master/isa/rv64si/csr.S
                                        # OR alternatively use the trick of setting exception handler trailing the operation
                                        # see: https://github.com/riscv/riscv-test-env/blob/master/p/riscv_test.h
                                        # >      la t0, 1f
                                        # >      csrw mtvec, t0
                                        # >      ... code that might fail due to lack of support in CPU
                                        # > 1:

        la      a0, msg_m_hello         # DEBUG print
        call    printf                  # DEBUG print

        mv      a0, a1                  # a1 contains the address of FDT header, pass it to kinit
        call    kinit                   # kinit() will set up the kernel for further operations and return

1:      wfi                             # after kinit() is done, halt this hart until the timer gets called, all the remaining
        j       1b                      # kernel ops will be orchestrated from the timer
                                        # parked hart will sleep waiting for interrupt


### Trap Tables & Dispatchers #################################################
#
                                        # from: sifive-interrupt-cookbook-v1p2.pdf
                                        # 2.1.3 Early Boot: Setup mtvec Register
                                        # > For sanity's sake we set up an early trap vector that just does nothing.
                                        # > If you end up here then there's a bug in the early boot code somewhere.
early_trap_vector:
1:      csrr    t0, mcause
        csrr    t1, mepc
        csrr    t2, mtval
        j       1b                      # loop indefinitely

                                        # 3.1.12 Machine Trap-Vector Base-Address Register (mtvec)
                                        # > When MODE=Vectored, all synchronous exceptions into machine mode cause
                                        # > the pc to be set to the address in the BASE field, whereas interrupts cause
                                        # > the pc to be set to the address in the BASE field plus four times the interrupt cause number.
                                        # >
                                        # > When vectored interrupts are enabled, interrupt cause 0, which corresponds to user-mode
                                        # > software interrupts, are vectored to the same location as synchronous exceptions.
                                        # > This ambiguity does not arise in practice, since user-mode software interrupts are
                                        # > either disabled or delegated to a less-privileged mode.


                                        # from: https://stackoverflow.com/questions/58726942/risc-v-exceptions-vs-interrupts
                                        # @TODO: figure out correct section as stackoverflow answer does not match the v1.10 document anymore :(
                                        # > Section 3.1.15 of the privileged RISC-V specification:
                                        # > when an interrupt is raised MEPC points to the first uncompleted instruction AND
                                        # > when an exception is raised MEPC points to the instruction that raised an exception.
                                        # >
                                        # > Section 1.6 of the unprivileged RISC-V specification defines that:
                                        # > a) exceptions are raised by instructions and
                                        # > b) interrupts are raised by external events.
                                        # >
                                        # > When an (synchronous) exception is raised, the triggering instruction cannot be completed properly.
                                        # > Hence, there are two possibilities for the return address: either the instruction itself or
                                        # > the following instruction. Both solutions make sense. If it points to the instruction itself
                                        # > it is easier to determine the problem and react accordingly. If it points to the next instruction,
                                        # > the address need not be incremented when returning from the exception handler.
                                        # >
                                        # > (Asynchronous) interrupts are different, they break the stream of executed instructions of
                                        # > an independent thread. Therefore, there is only one reasonable solution for the return address:
                                        # > the first instruction that has not completed yet. Thus, when returning from the interrupt handler,
                                        # > the execution continues exactly where it was interrupted.
.globl trap_vector
trap_vector:                            # 3.1.20 Machine Cause Register (mcause), Table 3.6: Machine cause register (mcause) values after trap.
        j exception_dispatch            #  0: user software interrupt OR _exception_ (See note in 3.1.12: Machine Trap-Vector Base-Address Register)
        j interrupt_noop                #  1: supervisor software interrupt
        j interrupt_noop                #  2: reserved
        j interrupt_noop                #  3: machine software interrupt
        j interrupt_timer               #  4: user timer interrupt
        j interrupt_timer               #  5: supervisor timer interrupt
        j interrupt_noop                #  6: reserved
        j k_interrupt_timer             #  7: machine timer interrupt
        j interrupt_noop                #  8: user external interrupt
        j interrupt_noop                #  9: supervisor external interrupt
        j interrupt_noop                # 10: reserved
        j interrupt_noop                # 11: machine external interrupt

exception_vector:                       # 3.1.20 Machine Cause Register (mcause), Table 3.6: Machine cause register (mcause) values after trap.
        j exception                     #  0: instruction address misaligned
        j exception                     #  1: instruction access fault
        j exception                     #  2: illegal instruction
        j exception                     #  3: breakpoint
        j exception                     #  4: load address misaligned
        j exception                     #  5: load access fault
        j exception                     #  6: store/AMO address misaligned
        j exception                     #  7: store/AMO access fault
        j syscall_dispatch              #  8: environment call from U-mode
        j syscall_dispatch              #  9: environment call from S-mode
        j exception                     # 10: reserved
        j syscall_dispatch              # 11: environment call from M-mode
        j exception                     # 12: instruction page fault
        j exception                     # 13: load page fault
        j exception                     # 14: reserved
        j exception                     # 15: store/AMO page fault
        j exception                     # 16: reserved
exception_vector_end:

exception_dispatch:
        stackalloc_x 32                 # allocate enough space to potentially store _all_ registers
                                        # in case the following handlers would need to save clobbered state
        sx      x5, 5, (sp)             # t0 :: x5
        sx      x6, 6, (sp)             # t1 :: x6
        la      t0, exception_vector
        csrr    t1, mcause
        slli    t1, t1, 2
        add     t0, t0, t1              # jump -> exception_vector + mcause * 4
                                        # check out-of-bounds
        la      t1, exception_vector_end
        bgeu    t0, t1, exception_epilogue
        jr      t0

syscall_dispatch:                       # a7 contains syscall index
        la      t0, syscall_vector
        li      t1, 5                   # TODO: 5 is the number of entries in syscall_vector. We need to find a way to
                                        # unhardcode this number and derive it from the actual table
        bgeu    a7, t1, syscall_epilogue
        slli    t1, a7, LOG2_SIZEOF_PTR
        add     t0, t0, t1              # read a pointer from syscall_vector + a7 * sizeof(ptr)
        lx      t1, 0, (t0)             # dereference that pointer into a function address
        jalr    t1                      # call a function at that address
        j       syscall_epilogue

exception_epilogue:
.if HALT_ON_EXCEPTION == 1
1:      j       1b
.endif
syscall_epilogue:
        csrr    t0, mepc                # mepc points to the instruction that raised an exception
        addi    t0, t0, 4               # move mepc by the size of a single RISC-V instruction (always 4 bytes in RV32, RV64, RV128)
        csrw    mepc, t0                # to point to the next instruction in order to continue execution.

        lx      x5, 5, (sp)             # t0 :: x5
        lx      x6, 6, (sp)             # t1 :: x6
        stackfree_x 32                  # see exception_dispatch, where we allocated 32 for _all_ registers
        mret

interrupt_epilogue:
        mret


### Exception, Interrupt & Syscall Handlers ###################################
#

exception:
                                        # no need to save: x0 - constant
                                        #                  t0, t1 - done in exception_dispatch
                                        #                  sp, gp, tp - not changed
                                        #                  s0..s7 - preserved inside functions
        sx      x1,   1, (sp)           # ra :: x1
        sx      x7,   7, (sp)           # t2 :: x7
        sx      x10, 10, (sp)           # a0 :: x10
        sx      x11, 11, (sp)
        sx      x12, 12, (sp)
        sx      x13, 13, (sp)
        sx      x14, 14, (sp)
        sx      x15, 15, (sp)
        sx      x16, 16, (sp)
        sx      x17, 17, (sp)           # a7 :: x17
        sx      x28, 28, (sp)           # t3 :: x28
        sx      x29, 29, (sp)
        sx      x30, 30, (sp)
        sx      x31, 31, (sp)           # t6 :: x31

        csrr    t0, mcause
        csrr    t1, mepc
        csrr    t2, mtval
        la      t3, user_payload
        la      t4, stack_top
        stackalloc_x 5
        sx      t0, 0, (sp)
        sx      t1, 1, (sp)
        sx      t2, 2, (sp)
        sx      t3, 3, (sp)
        sx      t4, 4, (sp)
        la      a0, msg_exception
        mv      a1, sp
        call    printf
        stackfree_x 5

        lx      x1,   1, (sp)           # ra :: x1
        lx      x7,   7, (sp)           # t2 :: x7
        lx      x10, 10, (sp)           # a0 :: x10
        lx      x11, 11, (sp)
        lx      x12, 12, (sp)
        lx      x13, 13, (sp)
        lx      x14, 14, (sp)
        lx      x15, 15, (sp)
        lx      x16, 16, (sp)
        lx      x17, 17, (sp)           # a7 :: x17
        lx      x28, 28, (sp)           # t3 :: x28
        lx      x29, 29, (sp)
        lx      x30, 30, (sp)
        lx      x31, 31, (sp)           # t6 :: x31
        j       exception_epilogue

interrupt_noop:
        j       interrupt_epilogue

interrupt_timer:
        j       interrupt_epilogue

k_interrupt_timer:
        call    kernel_timer_tick
        j       interrupt_epilogue

.globl kprints
kprints:
        stackalloc_x 1
        sx      ra, 1, (sp)
        call    printf
        lx      ra, 1, (sp)
        stackfree_x 1
        ret

### User payload = code + readonly data for U-mode ############################
#
.macro  macro_syscall nr
        li      a7, \nr                 # for fun let's pretend syscall is kinda like Linux: syscall nr in a7, other arguments in a0..a6
        ecall
.endm
.macro  macro_sys_poweroff exit_code
        li      a0, \exit_code
        macro_syscall 0
.endm
.macro  macro_sys_print addr
        la      a0, \addr
        macro_syscall 4
.endm

.section .user_text                     # at least in QEMU 5.1 memory protection seems to work on 4KiB page boundaries,
                                        # so store user payload part in its own section, which is aligned on 4KiB to make
                                        # the further experiments more predictable
.globl user_payload
user_payload:
.globl user_entry_point
user_entry_point:
        nop                             # no-operation instructions here help to distinguish between
        nop                             # illegal instruction -vs- page fault due to missing e(X)ecute access flag
        nop                             #
        nop                             # 4 nops instead of one, to align the code below on 0x10
                                        # which makes instruction address easier to follow in case of an exception

        call u_main
        ret

.globl user_entry_point2
user_entry_point2:
        nop                             # no-operation instructions here help to distinguish between
        nop                             # illegal instruction -vs- page fault due to missing e(X)ecute access flag
        nop                             #
        nop                             # 4 nops instead of one, to align the code below on 0x10
                                        # which makes instruction address easier to follow in case of an exception

        call u_main2
        ret

.globl sys_puts
sys_puts:
        stackalloc_x 1
        sx      ra, 1, (sp)
        macro_syscall 4
        lx      ra, 1, (sp)
        stackfree_x 1
        ret

a_string_in_user_mem:
        .string "This is a test string in user memory"
.globl a_string_in_user_mem_ptr
a_string_in_user_mem_ptr:
        pointer a_string_in_user_mem
.globl msg_m_hello_ptr
msg_m_hello_ptr:
        pointer msg_m_hello

msg_call_printf:
        .string "Illegal call to function in protected memory: "  # XXX: unused

### Data ######################################################################
#

.section .rodata
.globl rodata
rodata:
msg_m_hello:
        .string "Hello from M-mode!\n"
msg_exception:
        .string "Exception occurred, mcause:%p mepc:%p mtval:%p (user payload at:%p, stack top:%p).\n"
