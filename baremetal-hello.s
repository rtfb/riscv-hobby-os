.include "machine-word.inc"
.equ STACK_PER_HART,    64 * REGBYTES
.align 4
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
        addi    sp, sp, -19 * REGBYTES  # allocate 19 arguments on stack
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

                                        # print symbols (only on the 1st hart)
        bnez    s2, 2f
        la      a0, print_symbols_str
        la      a1, print_symbols_arg
        call    printf

2:      la      t0, current_hart
        addi    t1, s2, 1
        sx      t1, 0, (t0)             # trigger the next hart

park:
        wfi
        j       park

.section .rodata
rodata_start:
print_registers_str:
        .string "\nRegisters: mhartid=%p ra=%p gp=%p tp=%p fp=%p pc=%p\n\t\ta0=%p a1=%p a2=%p a3=%p a4=%p a5=%p a6=%p a7=%p\n\t\tt0=%p t1=%p t2=%p\n\t\tsp(entree)=%p sp(current)=%p\n"
print_symbols_str:
        .string "Symbols: _start=%p rodata_start=%p data_start=%p printf=%p this-str=%p stack_top=%p _end=%p\n"
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

