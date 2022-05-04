.include "src/machine-word.inc"
.equ STACK_PER_HART,    64 * REGBYTES
.equ HALT_ON_EXCEPTION, 0
.equ CLINT0_BASE_ADDRESS, 0x2000000
.equ BOOT_HART_ID, 0

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

.ifndef NO_S_MODE
        csrwi   mideleg, 0              # disable trap delegation, all interrupts and exceptions will be handled in machine mode
        csrwi   medeleg, 0              # 3.1.8 Machine Trap Delegation Registers (medeleg and mideleg)
                                        # > In systems with S-mode, the medeleg and mideleg registers must exist, and
                                        # > setting a bit in medeleg or mideleg will delegate the corresponding trap,
                                        # > when occurring in S-mode or U-mode, to the S-mode trap handler. In systems
                                        # > without S-mode, the medeleg and mideleg registers should not exist.
.endif

                                        # setup stack pointer:
        la      t0, stack_top           # set it at stack_top for hart0,
        csrr    t1, mhartid             # at stack_top+512 for hart1, etc.
        li      t2, 512
        mul     t1, t1, t2
        add     t0, t0, t1
        mv      sp, t0

        # only allow mhartid==BOOT_HART_ID to jump to the init_segments code;
        # all other harts continue to run hart_sync_code, then unconditionally
        # jump to init.
        li      t0, BOOT_HART_ID
        csrr    t1, mhartid
        beq     t0, t1, init_segments

hart_sync_code:
        # Only non-boot harts run this code:
        # Step 1: every non-boot hart writes 1 to MSIP register:
        csrr    t2, mhartid   # t2 = hart ID
        li      t0, 1
        li      t1, CLINT0_BASE_ADDRESS  # t1 = Core-Local Interrupt Controller base address
        slli    t2, t2, 2     # t2 = hartID*4
        add     t1, t1, t2    # t1 = address of MSIP mem-mapped register
        sw      t0, (t1)      # MSIP is 32 bits wide on all XLENs, so use sw
        fence   w,rw

        # Step 2: now busy-wait until the boot hart will clear that bit. When
        # it does, we're safe to proceed:
1:
        lw      t0, (t1)
        bnez    t0, 1b

        # now that we've gone through hart sync dance, continue to higher level
        # init code where each hart is allowed to run:
        j       init

init_segments:

        # This section of code between init_segments and synchronize_harts will
        # only be executed by a hart with id==BOOT_HART_ID. This ensures that
        # the .bss and .data segments (and other, as may become necessary) are
        # initialized once only.

        la      t0, bss_start
        la      t1, bss_end
        bgeu    t0, t1, synchronize_harts
clean_bss_loop:
        sw      zero, (t0)
        addi    t0, t0, 4
        bltu    t0, t1, clean_bss_loop

        # Now that the memory segments are init'ed, the boot hart will
        # synchronize all other harts: loop from 1 to NUM_HARTS, and for all of
        # them do the following: read its MSIP register, wait until they all
        # are 1. When all 1s are read from all harts, write 0s back to all of
        # them, allowing them all to proceed.
synchronize_harts:
        li      t0, 0                   # t0 = hart ID
        li      t1, CLINT0_BASE_ADDRESS # t1 = Core-Local Interrupt Controller base address
        li      t5, NUM_HARTS           # t4 = NUM_HARTS
        li      t6, BOOT_HART_ID        # t6 = boot hart ID, so that we can skip over it

        # if we're running on a single-harted machine, there's nothing to sync,
        # so jump over:
        beq     t0, t5, init

        # Read loop: wait for MSIP=1 from each hart
per_hart_read_loop:
        beq     t0, t6, skip_boot_hart
        slli    t2, t0, 2               # t2 = hartID*4
        add     t3, t1, t2              # t3 = address of MSIP mem-mapped register
        lw      t4, (t3)                # MSIP is 32 bits wide on all XLENs, so use lw
        beq     t4, zero, per_hart_read_loop

skip_boot_hart:
        addi    t0, t0, 1
        blt     t0, t5, per_hart_read_loop    # jump if hartID < NUM_HARTS

        # Write loop: write MSIP=0 for each hart
        li      t0, 0                   # t0 = hart ID
per_hart_write_loop:
        beq     t0, t6, skip_boot_hart2
        slli    t2, t0, 2               # t2 = hartID*4
        add     t3, t1, t2              # t3 = address of MSIP mem-mapped register
        sw      zero, (t3)

skip_boot_hart2:
        addi    t0, t0, 1
        blt     t0, t5, per_hart_write_loop  # jump if hartID < NUM_HARTS

init:

                                        # @TODO: check if user mode is supported
                                        # see: https://github.com/riscv/riscv-tests/blob/master/isa/rv64si/csr.S
                                        # OR alternatively use the trick of setting exception handler trailing the operation
                                        # see: https://github.com/riscv/riscv-test-env/blob/master/p/riscv_test.h
                                        # >      la t0, 1f
                                        # >      csrw mtvec, t0
                                        # >      ... code that might fail due to lack of support in CPU
                                        # > 1:

        mv      a0, a1                  # a1 contains the address of FDT header, pass it to kinit
        call    kinit                   # kinit() will set up the kernel for
                                        # further operations and never return.

        j exception                     # jump to exception here, just in case


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
.balign 64
trap_vector:                            # 3.1.20 Machine Cause Register (mcause), Table 3.6: Machine cause register (mcause) values after trap.
        j exception_dispatch            #  0: user software interrupt OR _exception_ (See note in 3.1.12: Machine Trap-Vector Base-Address Register)
.balign 4
        j interrupt_noop                #  1: supervisor software interrupt
.balign 4
        j interrupt_noop                #  2: reserved
.balign 4
        j interrupt_noop                #  3: machine software interrupt
.balign 4
        j interrupt_timer               #  4: user timer interrupt
.balign 4
        j interrupt_timer               #  5: supervisor timer interrupt
.balign 4
        j interrupt_noop                #  6: reserved
.balign 4
        j k_interrupt_timer             #  7: machine timer interrupt
.balign 4
        j interrupt_noop                #  8: user external interrupt
.balign 4
        j interrupt_noop                #  9: supervisor external interrupt
.balign 4
        j interrupt_noop                # 10: reserved
.balign 4
        j interrupt_noop                # 11: machine external interrupt

exception_vector:                       # 3.1.20 Machine Cause Register (mcause), Table 3.6: Machine cause register (mcause) values after trap.
.balign 4
        j exception                     #  0: instruction address misaligned
.balign 4
        j exception                     #  1: instruction access fault
.balign 4
        j exception                     #  2: illegal instruction
.balign 4
        j exception                     #  3: breakpoint
.balign 4
        j exception                     #  4: load address misaligned
.balign 4
        j exception                     #  5: load access fault
.balign 4
        j exception                     #  6: store/AMO address misaligned
.balign 4
        j exception                     #  7: store/AMO access fault
.balign 4
        j syscall_dispatch              #  8: environment call from U-mode
.balign 4
        j syscall_dispatch              #  9: environment call from S-mode
.balign 4
        j exception                     # 10: reserved
.balign 4
        j syscall_dispatch              # 11: environment call from M-mode
.balign 4
        j exception                     # 12: instruction page fault
.balign 4
        j exception                     # 13: load page fault
.balign 4
        j exception                     # 14: reserved
.balign 4
        j exception                     # 15: store/AMO page fault
.balign 4
        j exception                     # 16: reserved
.balign 4
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
        li      t1, 21                  # TODO: 21 is the number of entries in syscall_vector. We need to find a way to
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

# kprintf is called with args in a0..a7, but printf expects to get fmt string
# in a0 and a pointer to the rest of args in a1. So push a1..a7 to stack and
# pass a pointer to that location in a1.
.globl kprintf
kprintf:
        stackalloc_x 8
        mv      t0, sp
        sx      a1, 0, (sp)
        sx      a2, 1, (sp)
        sx      a3, 2, (sp)
        sx      a4, 3, (sp)
        sx      a5, 4, (sp)
        sx      a6, 5, (sp)
        sx      a7, 6, (sp)
        sx      ra, 7, (sp)
        mv      a1, t0
        call    printf
        lx      ra, 7, (sp)
        stackfree_x 8
        ret

.globl park_hart
park_hart:
loop:      wfi                  # parked hart will sleep waiting for interrupt
           j       loop

### User payload = code + readonly data for U-mode ############################
#
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

.section .data
.globl data
bss_zero_loop_lock:
        .word 0
