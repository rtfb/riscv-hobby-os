.include "machine-word.inc"
.equ STACK_PER_HART,    64 * REGBYTES

# These addresses are taken from the SiFive E31 core manual[1], Chapter 8: Core
# Local Interruptor (CLINT)
# [1] https://static.dev.sifive.com/E31-RISCVCoreIP.pdf
.equ MTIME, 0x200bff8
.equ MTIMECMP, 0x2004000

# This was determined experimentally:
.equ ONE_SECOND, 10000000

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

# NB: only do timer in RV64 for now, will do RV32 later
.ifdef RISCV64
        # Set the timer: read current time from mtime, add delta time to it,
        # and write the result to mtimecmp:
        li      t2, MTIME
        ld      t2, 0(t2)               # read from mtime mmapped register
        li      t4, (5*ONE_SECOND)      # t4 = 5s, use a register because the value is too big for immediate instruction
        add     t2, t2, t4              # t2 = mtime + 5s
        li      t3, MTIMECMP
        sd      t2, 0(t3)               # write t2 to mtimecmp


        # Enable interrupts by setting required values to mstatus, mtvec and mie:
        li      t0, 0x8                 # make a mask for 3rd bit
        csrrs   t1, mstatus, t0         # set MIE (M-mode Interrupts Enabled) bit in mstatus reg

        # store timer_handler directly to tvec. This will leave the mode bits
        # set to 0, which means direct mode, in which all exceptions call
        # timer_handler directly.
        la      t0, timer_handler
        csrrw   t1, mtvec, t0

        li      t0, 0x80                # mask for 7th bit
        csrrs   t1, mie, t0             # set MTIE (M-mode Timer Interrupt Enabled) bit
.endif

        # print registers
        stackalloc_x 19                 # allocate 19 register size arguments on stack
        sx      s2,  0,(sp)             # s2 contains mhartid
        sx      ra,  1,(sp)
        sx      gp,  2,(sp)
        sx      tp,  3,(sp)
        sx      fp,  4,(sp)
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

                                        # print symbols, but only on the 1st hart
        bnez    s2, 2f
        la      a0, print_symbols_str
        la      a1, print_symbols_arg
        call    printf

        # print M-mode registers, also only on the 1st hart
        stackalloc_x 5                  # allocate 5 register size arguments on stack
        sx      s2, 0,(sp)              # s2 contains mhartid
        csrr    t2, misa                # read misa (Machine ISA Register)
        sx      t2, 1,(sp)
        csrr    t2, mstatus             # read mstatus
        sx      t2, 2,(sp)
.ifdef RISCV64
        li      t2, MTIME
        ld      t2, 0(t2)               # read from mtime mmapped register
.else
        li      t2, 0
.endif
        sx      t2, 3,(sp)
.ifdef RISCV64
        li      t2, MTIMECMP
        ld      t2, 0(t2)               # read from mtimecmp mmapped register
.else
        li      t2, 0
.endif
        sx      t2, 4,(sp)
        la      a0, print_mregs_str
        mv      a1, sp
        call    printf
        stackfree_x 5

2:      la      t0, current_hart
        addi    t1, s2, 1
        sx      t1, 0, (t0)             # trigger the next hart

park:
        wfi
        j       park


# NB: only do timer in RV64 for now, will do RV32 later
.ifdef RISCV64
timer_handler:
        # print a few interesting registers from the timer handler
        stackalloc_x 6                  # allocate 6 register size arguments on stack
        csrr    t2, mcause
        sx      t2, 0,(sp)
        csrr    t2, mepc
        sx      t2, 1,(sp)
        csrr    t2, mtval
        sx      t2, 2,(sp)
        csrr    t2, mstatus
        sx      t2, 3,(sp)
        li      t2, MTIME
        ld      t2, 0(t2)
        sx      t2, 4,(sp)
        li      t2, MTIMECMP
        ld      t2, 0(t2)
        sx      t2, 5,(sp)
        la      a0, print_timer_str
        mv      a1, sp
        call    printf
        stackfree_x 6

        # now reset the timer for the future again:
        li      t2, MTIME
        ld      t2, 0(t2)
        li      t4, (3*ONE_SECOND)      # t4 = 3s
        add     t2, t2, t4              # t2 = mtime + 3s
        li      t3, MTIMECMP
        sd      t2, 0(t3)               # write t2 to mtimecmp

        # return from the handler
        mret
.endif

.section .rodata
rodata_start:
print_registers_str:
        .string "\nRegisters: mhartid=%p ra=%p gp=%p tp=%p fp=%p pc=%p\n\t\ta0=%p a1=%p a2=%p a3=%p a4=%p a5=%p a6=%p a7=%p\n\t\tt0=%p t1=%p t2=%p\n\t\tsp(entree)=%p sp(current)=%p\n"
print_symbols_str:
        .string "Symbols: _start=%p rodata_start=%p data_start=%p printf=%p this-str=%p stack_top=%p _end=%p\n"
print_mregs_str:
        .string "\nM-mode registers: mhartid=%p misa=%p mstatus=%p mtime=%p mtimecmp=%p\n"
print_timer_str:
        .string "\nHullo from timer! mcause=%p mepc=%p mtval=%p mstatus=%p mtime=%p mtimecmp=%p\n"
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

