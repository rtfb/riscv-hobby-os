.include "machine-word.inc"
.equ STACK_PER_HART,    64 * REGBYTES

.macro  syscall nr
        li      a7, \nr                 # for fun let's pretend syscall is kinda like Linux: syscall nr in a7, other arguments in a0..a6
        ecall
.endm

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
        csrwi   mie, 0                  # It is recommended to disable interrupts globally using mstatus.mie prior to changing mtvec.
                                        # For sanity's sake we set up an early trap vector that just does nothing.
        la      t0, early_trap_vector
        csrw    mtvec, t0

        csrwi   mstatus, 0
        csrwi   mideleg, 0
        csrwi   medeleg, 0

                                        # @TODO: setup Global Pointer

                                        # from: https://github.com/sifive/freedom-metal/master/src/entry.S & metal.ramrodata.lds
                                        # The absolute first thing that must happen is configuring the global
                                        # pointer register, which must be done with relaxation disabled because
                                        # it's not valid to obtain the address of any symbol without GP configured.
                                        #       . = ALIGN(8);
                                        #       PROVIDE( __global_pointer$ = . + 0x800 );
                                        #       *(.sdata .sdata.* .sdata2.*)

                                        # from: https://gnu-mcu-eclipse.github.io/arch/riscv/programmer/
                                        # The gp (Global Pointer) register is a solution to further optimise memory accesses within a single 4KB region.
                                        # The region size is 4K because RISC-V immediate values are 12-bit signed values,
                                        # since the values are signed, the __global_pointer$ must point to the middle of the region.
                                        #       PROVIDE( __global_pointer$ = . + (4K / 2) );
                                        #       *(.sdata .sdata.*)
        # .option push                  # requires that we disable linker relaxations (or it will be relaxed to `mv gp, gp`).
        # .option norelax
        # la  gp, __global_pointer$
        # .option pop

                                        # @TODO: process Device Tree, if available
                                        # judging by: https://github.com/qemu/qemu/blob/master/hw/riscv/sifive_u.c
                                        # pointer to Device Tree is passed in a1 register @TODO: figure out, if this behavior is SiFive specific


        csrr    a0, mhartid             # read hardware thread id (`hart` stands for `hardware thread`)

                                        # @TODO: support multi-core

        beqz    a0, single_core         # park all harts except the 1st one
1:      wfi                             # parked hart will sleep waiting for interrupt
        j       1b

single_core:                            # only the 1st hart past this point
        la      sp, stack_top
        # li      t0, STACK_PER_HART    # setup stack pointer for each hardware thread
        # mul     t0, t0, a0
        # sub     sp, sp, t0            # sp -= stack_size * hart_id

                                        # 3.1.12 Machine Trap-Vector Base-Address Register (mtvec)
                                        # When MODE=Vectored, all synchronous exceptions into machine mode cause
                                        # the pc to be set to the address in the BASE field, whereas interrupts cause
                                        # the pc to be set to the address in the BASE field plus four times the interrupt cause number.
                                        # When vectored interrupts are enabled, interrupt cause 0, which corresponds to user-mode
                                        # software interrupts, are vectored to the same location as synchronous exceptions.
                                        # This ambiguity does not arise in practice, since user-mode software interrupts are
                                        # either disabled or delegated to a less-privileged mode.


                                        # set up exception, interrupt & syscall trap vector
        la      t0, trap_vector
                                        # 3.1.12 Machine Trap-Vector Base-Address Register (mtvec), Table 3.5: Encoding of mtvec MODE field.
                                        # trap vector mode is encoded in 2 bits: Direct = 0b00, Vectored = 0b01
                                        # and is stored in 0:1 bits of mtvect CSR (mtvec.mode)
        ori     t0, t0, 0b01            # mtvec.mode |= 0b01; trap_vector in t0 is 4 byte aligned, last two bits are zero
        csrw    mtvec, t0

                                        # @TODO: setup physical memory protection (PMP) for user mode
                                        # 3.6 Physical Memory Protection

        la      a0, print_m_hello_str   # DEBUG print
        call    printf                  # DEBUG print

                                        # 3.1.7 Privilege and Global Interrupt-Enable Stack in mstatus register
                                        # The MRET, SRET, or URET instructions are used to return from traps in M-mode, S-mode, or U-mode respectively.
                                        # When executing an xRET instruction, supposing x PP holds the value y, x IE is set to xPIE;
                                        # the privilege mode is changed to y; xPIE is set to 1; and xPP is set to U (or M if user-mode is not supported).

                                        # 3.2.2 Trap-Return Instructions
                                        # An xRET instruction can be executed in privilege mode x or higher,
                                        # where executing a lower-privilege xRET instruction will pop
                                        # the relevant lower-privilege interrupt enable and privilege mode stack.
                                        # In addition to manipulating the privilege stack as described in Section 3.1.7,
                                        # xRET sets the pc to the value stored in the x epc register.

                                        # switch from Machine to User mode:
                                        # set Machine Previous Privelege to User -> mstatus.mpp = User
                                        # set Machine Exception Program Counter  -> mepc = entree_point
                                        # call MRET

                                        # 1.3 Privelege Levels, Table 1.1: RISC-V privilege levels.
                                        # privelege levels are encoded in 2 bits: User = 0b00, Supervisor = 0b01, Machine = 0b11
        li      t0, (0b11 << 11)        # and stored in 11:12 bits of mstatus CSR (mstatus.mpp)
        csrc    mstatus, t0             # mstatus.mpp &= ~0b11; clear mstatus.mpp bits to 0 with t0 containing mask for bits to clear

                                        # set entry point address that mret will jump to
        la      t0, user_entry_point
        csrw    mepc, t0

        mret                            # mret will switch the mode stored in mstatus.mpp and jump to address stored in mepc CSR


### Trap Tables & Dispatchers #################################################
#
                                        # from: sifive-interrupt-cookbook-v1p2.pdf
                                        # 2.1.3 Early Boot: Setup mtvec Register
                                        # For sanity's sake we set up an early trap vector that just does nothing.
                                        # If you end up here then there's a bug in the early boot code somewhere.
early_trap_vector:
1:      csrr    t0, mcause
        csrr    t1, mepc
        csrr    t2, mtval
        j       1b                      # loop indefinitely

                                        # 3.1.12 Machine Trap-Vector Base-Address Register (mtvec)
                                        # When MODE=Vectored, all synchronous exceptions into machine mode cause
                                        # the pc to be set to the address in the BASE field, whereas interrupts cause
                                        # the pc to be set to the address in the BASE field plus four times the interrupt cause number.
                                        # 
                                        # When vectored interrupts are enabled, interrupt cause 0, which corresponds to user-mode
                                        # software interrupts, are vectored to the same location as synchronous exceptions.
                                        # This ambiguity does not arise in practice, since user-mode software interrupts are
                                        # either disabled or delegated to a less-privileged mode.


                                        # from: https://stackoverflow.com/questions/58726942/risc-v-exceptions-vs-interrupts
                                        # @TODO: figure out correct section as stackoverflow answer does not match the v1.10 document anymore :(
                                        # Section 3.1.15 of the privileged RISC-V specification:
                                        # when an interrupt is raised MEPC points to the first uncompleted instruction AND
                                        # when an exception is raised MEPC points to the instruction that raised an exception.
                                        #
                                        # Section 1.6 of the unprivileged RISC-V specification defines that:
                                        # a) exceptions are raised by instructions and
                                        # b) interrupts are raised by external events.
                                        #
                                        # When an (synchronous) exception is raised, the triggering instruction cannot be completed properly.
                                        # Hence, there are two possibilities for the return address: either the instruction itself or
                                        # the following instruction. Both solutions make sense. If it points to the instruction itself
                                        # it is easier to determine the problem and react accordingly. If it points to the next instruction,
                                        # the address need not be incremented when returning from the exception handler.
                                        # 
                                        # (Asynchronous) interrupts are different, they break the stream of executed instructions of
                                        # an independent thread. Therefore, there is only one reasonable solution for the return address:
                                        # the first instruction that has not completed yet. Thus, when returning from the interrupt handler,
                                        # the execution continues exactly where it was interrupted.
trap_vector:                            # 3.1.20 Machine Cause Register (mcause), Table 3.6: Machine cause register (mcause) values after trap.
        j exception_dispatch            #  0: user software interrupt OR _exception_ (See note in 3.1.12: Machine Trap-Vector Base-Address Register)
        j interrupt_noop                #  1: supervisor software interrupt
        j interrupt_noop                #  2: reserved
        j interrupt_noop                #  3: machine software interrupt
        j interrupt_timer               #  4: user timer interrupt
        j interrupt_timer               #  5: supervisor timer interrupt
        j interrupt_noop                #  6: reserved
        j interrupt_timer               #  7: machine timer interrupt
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

syscall_vector:                         # for fun let's pretend syscall table is kinda like 32bit Linux on x86,
                                        # /usr/include/asm/unistd_32.h: __NR_restart_syscall 0, __NR_exit 1, _NR_fork 2, __NR_read 3, __NR_write 4
        j syscall0
        j syscall1
        j syscall2
        j syscall3
        j syscall4
syscall_vector_end:

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
        slli    t1, a7, 2
        add     t0, t0, t1              # jump -> syscall_vector + (a7-1) * 4
        la      t1, syscall_vector_end  # check out-of-bounds
        bgeu    t0, t1, syscall_epilogue
        jr      t0

exception_epilogue:
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
        la      t3, user_entry_point
        stackalloc_x 4
        sx      t0, 0, (sp)
        sx      t1, 1, (sp)
        sx      t2, 2, (sp)
        sx      t3, 3, (sp)
        la      a0, print_exception_str
        mv      a1, sp
        call    printf
        stackfree_x 4

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

syscall0:
syscall1:
syscall2:
syscall3:
syscall4:
        jal     prints
        j       syscall_epilogue


### User mode code ############################################################
#

user_entry_point:
        la      a0, print_u_hello_str
        syscall 4                       # 4 :: prints syscall

        csrr    a0, mhartid             # causes Illegal instruction (mcause=2) in User mode

        la      a0, print_u_hello_str
        call    printf

1:      wfi                             # park hart waiting for interrupt
        j       1b

### Data ######################################################################
#

.section .rodata
print_m_hello_str:
        .string "Hello from M-mode!\n"
print_u_hello_str:
        .string "Hello from U-mode!\n"
print_exception_str:
        .string "Exception occured, mcause:%p mepc:%p mtval:%p (U-mode code starts at:%p).\n"

.section .data

### CHECK if USER mode is supported
#   https://github.com/riscv/riscv-tests/blob/master/isa/rv64si/csr.S

#   # For RV64, make sure UXL encodes RV64.  (UXL does not exist for RV32.)
# #if __riscv_xlen == 64
#   # If running in M mode, use mstatus.MPP to check existence of U mode.
#   # Otherwise, if in S mode, then U mode must exist and we don't need to check.
# #ifdef __MACHINE_MODE
#   li t0, MSTATUS_MPP
#   csrc mstatus, t0
#   csrr t1, mstatus
#   and t0, t0, t1
#   bnez t0, 1f
# #endif
#   # If U mode is present, UXL should be 2 (XLEN = 64-bit)
#   TEST_CASE(18, a0, SSTATUS_UXL & (SSTATUS_UXL << 1), csrr a0, sstatus; li a1, SSTATUS_UXL; and a0, a0, a1)
# #ifdef __MACHINE_MODE
#   j 2f
# 1:
#   # If U mode is not present, UXL should be 0
#   TEST_CASE(19, a0, 0, csrr a0, sstatus; li a1, SSTATUS_UXL; and a0, a0, a1)
# 2:
# #endif
# #endif


### PMP SETUP EXAMPLE
#   https://github.com/riscv/riscv-test-env/blob/master/p/riscv_test.h

#   #define INIT_PMP                                         
#   la t0, 1f;                                               
#   csrw mtvec, t0;                                          
#   /* Set up a PMP to permit all accesses */                
#   li t0, (1 << (31 + (__riscv_xlen / 64) * (53 - 31))) - 1;
#   csrw pmpaddr0, t0;                                       
#   li t0, PMP_NAPOT | PMP_R | PMP_W | PMP_X;                
#   csrw pmpcfg0, t0;                                        
#   .align 2;                                                
# 1:



### TRAP EXMAPLES from Freedom SDK
#   https://github.com/sifive/freedom-metal/master/src/trap.S

#define METAL_MSTATUS_MIE_SHIFT 8
#define METAL_MSTATUS_MPP_M     3
#define METAL_MSTATUS_MPP_SHIFT 11

#define METAL_MTVEC_MODE_MASK   3

# /* void _metal_trap(int ecode)
#  *
#  * Trigger a machine-mode trap with exception code ecode
#  */
# .global _metal_trap
# .type _metal_trap, @function

# _metal_trap:

#     /* Store the instruction which called _metal_trap in mepc */
#     addi t0, ra, -1
#     csrw mepc, t0

#     /* Set mcause to the desired exception code */
#     csrw mcause, a0

#     /* Read mstatus */
#     csrr t0, mstatus

#     /* Set MIE=0 */
#     li t1, -1
#     xori t1, t1, METAL_MSTATUS_MIE_SHIFT
#     and t0, t0, t1

#     /* Set MPP=M */
#     li t1, METAL_MSTATUS_MPP_M
#     slli t1, t1, METAL_MSTATUS_MPP_SHIFT
#     or t0, t0, t1

#     /* Write mstatus */
#     csrw mstatus, t0

#     /* Read mtvec */
#     csrr t0, mtvec

#     /*
#      * Mask the mtvec MODE bits
#      * Exceptions always jump to mtvec.BASE regradless of the vectoring mode.
#      */
#     andi t0, t0, METAL_MTVEC_MODE_MASK

#     /* Jump to mtvec */
#     jr t0


# /*
#  * For sanity's sake we set up an early trap vector that just does nothing.
#  * If you end up here then there's a bug in the early boot code somewhere.
#  */
# .section .text.metal.init.trapvec
# .global early_trap_vector
# .align 2
# early_trap_vector:
#     .cfi_startproc
#     csrr t0, mcause
#     csrr t1, mepc
#     csrr t2, mtval
#     j early_trap_vector
#     .cfi_endproc
