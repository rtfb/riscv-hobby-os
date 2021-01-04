.include "machine-word.inc"
.equ STACK_PER_HART,    64 * REGBYTES

# These addresses are taken from the SiFive E31 core manual [1],
# Chapter 8: Core Local Interruptor (CLINT)
# [1] https://static.dev.sifive.com/E31-RISCVCoreIP.pdf
.equ MTIME,             0x200bff8
.equ MTIMECMP_BASE,     0x2004000

# Based on SIFIVE_CLINT_TIMEBASE_FREQ = 10000000 value from QEMU SiFive CLINT implementation [2],
# [2] https://github.com/qemu/qemu/blob/master/include/hw/intc/sifive_clint.h
.equ ONE_SECOND,        10000000

.balign 4
.section .text
.globl _start
_start:
                                        # save important registers for printing,
                                        # but skip s0 since it doubles as fp
        auipc   s1, 0                   # s1 <- pc
        csrr    s2, mhartid             # read hardware thread id (`hart` stands for `hardware thread`)

        mv      s3, sp
        mv      s4, sp
        mv      s5, t0
        mv      s6, t1
        mv      s7, t2

        la      sp, stack_top           # setup stack pointer
                                        # will allocate printf() arguments on the stack

        li      t0, STACK_PER_HART      # stack size per hart
        mul     t0, t0, s2
        sub     sp, sp, t0              # setup stack pointer per hart: sp -= stack_size_per_hart * mhartid
        mv      s4, sp

                                        # busy wait each mhartid
                                        # we expect that mhartids are sequential numbers starting at 0
1:      la      t0, current_hart
        lx      t0, 0,(t0)
        bne     t0, s2, 1b

        # print registers
        stackalloc_x 19                 # allocate 19 register size arguments on stack
        sx      s2,  0,(sp)             # s2 contains mhartid
        sx      ra,  1,(sp)
        sx      gp,  2,(sp)
        sx      tp,  3,(sp)
        sx      s0,  4,(sp)
        sx      s1,  5,(sp)             # s1 contains pc on entree

        sx      a0,  6,(sp)
        sx      a1,  7,(sp)
        sx      a2,  8,(sp)
        sx      a3,  9,(sp)
        sx      a4, 10,(sp)
        sx      a5, 11,(sp)
        sx      a6, 12,(sp)
        sx      a7, 13,(sp)

        sx      s5, 14,(sp)             # s5..s7 contains t0..t1 on entree
        sx      s6, 15,(sp)
        sx      s7, 16,(sp)

        sx      s3, 17,(sp)             # s3 contains sp on entree
        sx      s4, 18,(sp)             # s4 contains sp per hart

        la      a0, print_registers_str
        mv      a1, sp
        call    printf
        stackfree_x 19


                                        # print symbols, CSRs and setup timer, but only on the 1st hart
        bnez    s2, 2f
        la      a0, print_symbols_str
        la      a1, print_symbols_arg
        call    printf

                                        # print M-mode CSR registers
        stackalloc_x 5                  # allocate 5 register size arguments on stack
        sx      s2, 0,(sp)              # s2 contains mhartid
        csrr    t2, misa                # read misa (Machine ISA Register)
        sx      t2, 1,(sp)
        csrr    t2, mstatus             # read mstatus
        sx      t2, 2,(sp)
        li      t2, MTIME
        lx      t2, 0,(t2)              # read from memory-mapped mtime register, @TODO: support 64bit in RV32
        sx      t2, 3,(sp)
        li      t2, MTIMECMP_BASE
        csrr    t3, mhartid
        slli    t3, t3, 3
        add     t2, t2, t3              # mtimecmp += 8 * mhartid
        lx      t2, 0,(t2)              # read from memory-mapped mtimecmp_x64bit[mhartid] @TODO: support 64bit in RV32
        sx      t2, 4,(sp)
        la      a0, print_mregs_str
        mv      a1, sp
        call    printf
        stackfree_x 5

                                        # setup timer & enable interrupts
        li      a0, (5*ONE_SECOND)      # a0 = 5sec
        jal     set_timer_for_current_hart

                                        # enable interrupts by setting required values to mstatus, mtvec and mie:
        li      t0, 0x8                 # make a mask for 3rd bit
        csrrs   t1, mstatus, t0         # set MIE (M-mode Interrupts Enabled) bit in mstatus reg

                                        # store trap_vector to tvec, but first increment it by 1. This will set
                                        # the mode bits to 1, which means vectored mode, in which exceptions
                                        # jump to trap_vector+4*cause.
        la      t0, trap_vector
        addi    t0, t0, 1
        csrrw   t1, mtvec, t0

        li      t0, 0x80                # mask for 7th bit
        csrrs   t1, mie, t0             # set MTIE (M-mode Timer Interrupt Enabled) bit


2:      la      t0, current_hart
        addi    t1, s2, 1
        sx      t1, 0, (t0)             # trigger the next hart

park:
        wfi
        j       park

trap_vector:
        j noop_handler                  # offs  0: user software interrupt
        j noop_handler                  # offs  4: supervisor software interrupt
        j noop_handler                  # offs  8: reserved
        j noop_handler                  # offs  c: machine software interrupt
        j noop_handler                  # offs 10: user timer interrupt
        j noop_handler                  # offs 14: supervisor timer interrupt
        j noop_handler                  # offs 18: reserved
        j timer_handler                 # offs 1c: machine timer interrupt

noop_handler:
        mret

timer_handler:
        # print a few interesting registers from the timer handler
        stackalloc_x 7                  # allocate 7 register size arguments on stack
        csrr    t2, mhartid
        sx      t2, 0,(sp)
        csrr    t2, mcause
        sx      t2, 1,(sp)
        csrr    t2, mepc
        sx      t2, 2,(sp)
        csrr    t2, mtval
        sx      t2, 3,(sp)
        csrr    t2, mstatus
        sx      t2, 4,(sp)
        li      t2, MTIME               # read from memory-mapped mtime register, @TODO: support 64bit in RV32
        lx      t2, 0,(t2)
        sx      t2, 5,(sp)
        li      t2, MTIMECMP_BASE
        csrr    t3, mhartid
        slli    t3, t3, 3
        add     t2, t2, t3              # mtimecmp += 8 * mhartid
        lx      t2, 0,(t2)              # read from memory-mapped mtimecmp_x64bit[mhartid] @TODO: support 64bit in RV32
        sx      t2, 6,(sp)
        la      a0, print_timer_str
        mv      a1, sp
        call    printf
        stackfree_x 6

        li      a0, (3*ONE_SECOND)      # a0 = 3s
        jal     set_timer_for_current_hart

        # return from the handler
        mret


set_timer_for_current_hart:             # Set the timer: read current time from mtime, add delta time to it,
                                        # and write the result to mtimecmp of current hart:
                                        #       mtimecmp[mhartid] = mtime + delta_time
                                        # @param[in] a0 delta_time

        li      t5, MTIME
        li      t6, MTIMECMP_BASE
        csrr    t0, mhartid
        slli    t0, t0, 3
        add     t6, t6, t0              # mtimecmp += 8 * mhartid

.ifdef RISCV64
        ld      t0, 0(t5)               # read from mtime memory-mapped register
        add     t0, t0, a0              # t2 = mtime + 5sec
        sd      t0, 0(t6)               # write t2 to mtimecmp
.else
        lw      t0, 0(t5)               # read from mtime memory-mapped into t0:t1 pair
        lw      t1, 4(t5)
        li      t2, (5*ONE_SECOND)      # t2 = 5sec

                                        # add and write t1:t0 + t2 => t1:t4 into mtimecmp

                                        # unsigned int overlow detection based on example from RISC-V ISA section Integer Computational Instructions:
                                        #     add t0, t1, t2; bltu t0, t1, overflow.
        add     t4, t0, t2
        bgeu    t4, t0, 1f              # bgeu == !bltu
        addi    t1, t1, 1               # handle overflow
1:
                                        # mtimecmp write in RV32 based on example from RISC-V Priveleged ISA section Machine Timer Registers:
                                        #     li t0, -1; sw t0, mtimecmp; sw a1, mtimecmp+4; sw a0, mtimecmp.
        li      t0, -1
        sw      t0, 0(t6)               # 1) write unsigned (-1) into low 32bits of mtimecmp = no smaller than old value
        sw      t1, 4(t6)               # 1) write                 high 32bits into mtimecmp = no smaller than new value
        sw      t4, 0(t6)               # 1) write                  low 32bits into mtimecmp = new value
.endif
        ret


.section .rodata
rodata_start:
print_registers_str:
        .string "\nRegisters: mhartid=%p ra=%p gp=%p tp=%p fp=%p pc=%p\n\t\ta0=%p a1=%p a2=%p a3=%p a4=%p a5=%p a6=%p a7=%p\n\t\tt0=%p t1=%p t2=%p\n\t\tsp(entree)=%p sp(current)=%p\n"
print_symbols_str:
        .string "Symbols: _start=%p rodata_start=%p data_start=%p printf=%p this-str=%p stack_top=%p _end=%p\n"
print_mregs_str:
        .string "Machine CSRs: mhartid=%p misa=%p mstatus=%p mtime=%p mtimecmp=%p\n"
print_timer_str:
        .string "\nHullo from timer! mhartid=%p mcause=%p mepc=%p mtval=%p mstatus=%p mtime=%p mtimecmp=%p\n"
print_symbols_arg:
        pointer _start
        pointer rodata_start
        pointer data_start
        pointer printf
        pointer print_symbols_str
        pointer stack_top
        pointer _end
rodata_end:

.section .data
data_start:
current_hart:
        machineword 0

