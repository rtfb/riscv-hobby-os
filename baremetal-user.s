.include "machine-word.inc"
.equ STACK_PER_HART,    64 * REGBYTES

.balign 4
.section .text
.globl _start
_start:
        csrwi   mie, 0                  # it is recommended to disable interrupts globally using mstatus.mie prior to changing mtvec

                                        # From sifive-interrupt-cookbook-v1p2.pdf
                                        # set up a simple trap vector to catch anything that goes wrong early in the boot process
        la      t0, early_trap_vector
        csrw    mtvec, t0

        csrwi   mstatus, 0
        csrwi   mideleg, 0
        csrwi   medeleg, 0

                                        # @TODO: setup Global Pointer

                                        # From https://github.com/sifive/freedom-metal/master/src/entry.S & metal.ramrodata.lds
                                        # The absolute first thing that must happen is configuring the global
                                        # pointer register, which must be done with relaxation disabled because
                                        # it's not valid to obtain the address of any symbol without GP configured.
                                        #       . = ALIGN(8);
                                        #       PROVIDE( __global_pointer$ = . + 0x800 );
                                        #       *(.sdata .sdata.* .sdata2.*)

                                        # From https://gnu-mcu-eclipse.github.io/arch/riscv/programmer/
                                        # The gp (Global Pointer) register is a solution to further optimise memory accesses within a single 4KB region.
                                        # The region size is 4K because RISC-V immediate values are 12-bit signed values,
                                        # since the values are signed, the __global_pointer$ must point to the middle of the region.
                                        #       PROVIDE( __global_pointer$ = . + (4K / 2) );
                                        #       *(.sdata .sdata.*)
        # .option push                  # requires that we disable linker relaxations (or it will be relaxed to `mv gp, gp`).
        # .option norelax
        # la  gp, __global_pointer$
        # .option pop

        csrr    a0, mhartid             # read hardware thread id (`hart` stands for `hardware thread`)
        la      sp, stack_top           # setup stack pointer
                                        # will allocate printf() arguments on the stack

        li      t0, STACK_PER_HART      # stack size per hart
        mul     t0, t0, a0
        sub     sp, sp, t0              # setup stack pointer per hart: sp -= stack_size_per_hart * mhartid

                                        # @TODO: process Device Tree, if available
                                        # judging from https://github.com/qemu/qemu/blob/master/hw/riscv/sifive_u.c
                                        # pointer to Device Tree is passed in a1 register @TODO: figure out, if this behavior is SiFive specific

        la      t0, trap_vector
        addi    t0, t0, 1
        csrw    mtvec, t0


        bnez    a0, park                # park all harts except the 1st one


        la      a0, print_m_hello_str
        call    printf

                                        # Privelege ISA
                                        # 3.1.7 Privilege and Global Interrupt-Enable Stack in mstatus register
                                        # The MRET, SRET, or URET instructions are used to return from traps in M-mode, S-mode, or U-mode respectively.
                                        # When executing an xRET instruction, supposing x PP holds the value y, x IE is set to xPIE;
                                        # the privilege mode is changed to y; xPIE is set to 1; and xPP is set to U (or M if user-mode is not supported).

                                        # 3.2.2 Trap-Return Instructions
                                        # An xRET instruction can be executed in privilege mode x or higher,
                                        # where executing a lower-privilege xRET instruction will pop the relevant lower-privilege interrupt enable and privilege mode stack.
                                        # In addition to manipulating the privilege stack as described in Section 3.1.7,
                                        # xRET sets the pc to the value stored in the x epc register.


                                        # switch from Machine to User mode:
                                        # set Machine Previous Privelege to User -> mstatus.mpp = User
                                        # set Machine Exception Program Counter  -> mepc = entree_point
                                        # call MRET

                                        # privelege levels are encoded in 2 bits: User = 0b00, Supervisor = 0b01, Machine = 0b11
        li      t0, (0b11 << 11)        # mstatus.mpp is contained in 11:12 bits
        csrc    mstatus, t0             # clear mstatus.mpp to 0

                                        # set entree point adress that mret will jump to
        la      t0, user_entree_point
        csrw    mepc, t0

        mret                            # mret will switch the mode stored in mstatus.mpp and jump to address stored in mepc CSR

user_entree_point:

        # la      a0, print_hello_str
        # call    printf

        li      a7, 4
        la      a0, print_u_hello_str
        ecall                           # Environment call from U-mode (mcause=8)

        csrr    a0, mhartid             # causes Illegal instruction (mcause=2) in User mode

        la      a0, print_u_hello_str
        call    printf

park:
        wfi
        j       park

                                        # From sifive-interrupt-cookbook-v1p2.pdf
                                        # For sanity's sake we set up an early trap vector that just does nothing.
                                        # If you end up here then there's a bug in the early boot code somewhere.
early_trap_vector:
        csrr    t0, mcause
        csrr    t1, mepc
        csrr    t2, mtval
1:      j       1b

trap_vector:
        j exception_handler             #  0: user software interrupt
        j noop_handler                  #  1: supervisor software interrupt
        j noop_handler                  #  2: reserved
        j noop_handler                  #  3: machine software interrupt
        j timer_handler                 #  4: user timer interrupt
        j timer_handler                 #  5: supervisor timer interrupt
        j noop_handler                  #  6: reserved
        j timer_handler                 #  7: machine timer interrupt
        j noop_handler                  #  8: user external interrupt
        j noop_handler                  #  9: supervisor external interrupt
        j noop_handler                  # 10: reserved
        j noop_handler                  # 11: machine external interrupt

noop_handler:
        mret

timer_handler:
        mret

exception_handler:
        stackalloc_x 32
        sx      x0,   0, (sp)
        sx      x1,   1, (sp)
        sx      x2,   2, (sp)
        sx      x3,   3, (sp)
        sx      x4,   4, (sp)
        sx      x5,   5, (sp)
        sx      x6,   6, (sp)
        sx      x7,   7, (sp)
        sx      x8,   8, (sp)
        sx      x9,   9, (sp)
        sx      x10, 10, (sp)
        sx      x11, 11, (sp)
        sx      x12, 12, (sp)
        sx      x13, 13, (sp)
        sx      x14, 14, (sp)
        sx      x15, 15, (sp)
        sx      x16, 16, (sp)
        sx      x17, 17, (sp)
        sx      x18, 18, (sp)
        sx      x19, 19, (sp)
        sx      x20, 20, (sp)
        sx      x21, 21, (sp)
        sx      x22, 22, (sp)
        sx      x23, 23, (sp)
        sx      x24, 24, (sp)
        sx      x25, 25, (sp)
        sx      x26, 26, (sp)
        sx      x27, 27, (sp)
        sx      x28, 28, (sp)
        sx      x29, 29, (sp)
        sx      x30, 30, (sp)
        sx      x31, 31, (sp)

        csrr    t0, mcause
        li      t1, 8                   # environment call from U-mode
        beq     t0, t1, syscall
        li      t1, 9                   # environment call from S-mode
        beq     t0, t1, syscall
        li      t1, 11                  # environment call from M-mode
        beq     t0, t1, syscall

        stackalloc_x 4
        csrr    t0, mcause
        csrr    t1, mepc
        csrr    t2, mtval
        la      t3, user_entree_point
        sx      t0, 0, (sp)
        sx      t1, 1, (sp)
        sx      t2, 2, (sp)
        sx      t3, 3, (sp)
        la      a0, print_exception_str
        mv      a1, sp
        call    printf
        stackfree_x  4

exception_handler_ret:
        csrr    t0, mepc
        addi    t0, t0, 4
        csrw    mepc, t0

        lx      x0,   0, (sp)
        lx      x1,   1, (sp)
        lx      x2,   2, (sp)
        lx      x3,   3, (sp)
        lx      x4,   4, (sp)
        lx      x5,   5, (sp)
        lx      x6,   6, (sp)
        lx      x7,   7, (sp)
        lx      x8,   8, (sp)
        lx      x9,   9, (sp)
        lx      x10, 10, (sp)
        lx      x11, 11, (sp)
        lx      x12, 12, (sp)
        lx      x13, 13, (sp)
        lx      x14, 14, (sp)
        lx      x15, 15, (sp)
        lx      x16, 16, (sp)
        lx      x17, 17, (sp)
        lx      x18, 18, (sp)
        lx      x19, 19, (sp)
        lx      x20, 20, (sp)
        lx      x21, 21, (sp)
        lx      x22, 22, (sp)
        lx      x23, 23, (sp)
        lx      x24, 24, (sp)
        lx      x25, 25, (sp)
        lx      x26, 26, (sp)
        lx      x27, 27, (sp)
        lx      x28, 28, (sp)
        lx      x29, 29, (sp)
        lx      x30, 30, (sp)
        lx      x31, 31, (sp)
        stackfree_x 32
        mret

syscall:
        call    prints
        j       exception_handler_ret

.section .rodata
print_m_hello_str:
        .string "Hello from M-mode!\n"
print_u_hello_str:
        .string "Hello from U-mode!\n"
print_exception_str:
        .string "Exception occured, mcause:%p mepc:%p mtval:%p. (user_entree_point:%p)\n"

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
