.include "machine-word.inc"
.equ STACK_PER_HART,    64 * REGBYTES

.balign 4
.section .text
.globl _start
_start:
        csrr    a0, mhartid             # read hardware thread id (`hart` stands for `hardware thread`)
        la      sp, stack_top           # setup stack pointer
                                        # will allocate printf() arguments on the stack

        li      t0, STACK_PER_HART      # stack size per hart
        mul     t0, t0, a0
        sub     sp, sp, t0              # setup stack pointer per hart: sp -= stack_size_per_hart * mhartid

        bnez    a0, park

        la      a0, print_hello_str
        call    printf

park:
        wfi
        j       park

.section .rodata
print_hello_str:
        .string "Hello!\n"

.section .data

