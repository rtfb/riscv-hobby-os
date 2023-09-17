.include "src/machine-word.inc"
.balign 4
.section .text
.globl _start
_start:
        csrr    t0, mhartid             # read hardware thread id (`hart` stands for `hardware thread`)
        bnez    t0, halt                # run only on the first hardware thread (hartid == 0), halt all the other threads

        la      sp, stack_top           # setup stack pointer

        jal     main                    # call main()

halt:   j       halt

.global main
main:
        la      a0, hello_fmt
        la      a1, hello_arg
        call    kprintf                 # test kprintf()

                                        # calculate Fibonacci sequence
                                        # for (int i = 1; i < 15; i++)
                                        #     kprintf("fib(%d) = %d\n", i, fib(i));
        li      s0, 1
        li      s1, 15
1:      mv      a0, s0
        jal     fib                     # call fib() with a0 containing index of Fibonacci sequence.
                                        # fib() will return result in a0.

        addi    sp, sp, -8              # allocate 2 int arguments for kprintf() on the stack
        mv      a1, sp
        sw      s0, 0(a1)               # 1st kprintf argument s0 contains index i
        sw      a0, 4(a1)               # 2nd prinnt argument a0 contains result of fib(i)
        la      a0, fib_fmt
        call    kprintf                 # call kprintf() with a0 pointing to "fib(%d) = %d\n" pattern
                                        #                and a1 pointing to list of arguments [i, fib(i)] stored on stack
        addi    sp, sp, 8               # restore stack

        addi    s0, s0, 1               # i++
        blt     s0, s1, 1b              # loop while i < 15

        call    poweroff                # shutdown and exit QEMU, if possible

        ret

.global fib
fib:
        li      t0, 1
        li      t1, 2

        beq     a0, t0, 1f
        beq     a0, t1, 1f

        mv      t0, a0
        addi    a0, t0, -1              # calculate n-1

        stackalloc_x 2
        sx      ra, 0,(sp)
        sx      t0, 1,(sp)              # preserve t0, which contains our original argument
        jal     fib
        lx      ra, 0,(sp)
        lx      t0, 1,(sp)
        stackfree_x 2

        mv      t2, a0                  # t2 now contains fib(n-1)

        addi    a0, t0, -2              # calculate n-2

        stackalloc_x 2
        sx      ra, 0,(sp)
        sx      t2, 1,(sp)              # preserve t2, which has fib(n-1)
        jal     fib
        lx      ra, 0,(sp)
        lx      t2, 1,(sp)
        stackfree_x 2

        mv      t3, a0                  # t3 now contains fib(n-2)
        add     a0, t2, t3              # add them and jump to return
        j       2f

1:
        li      a0, 1

2:
        ret

.section .rodata
hello_str:
        .string "Hello world!!!\n"
hello_fmt:
        .string "%sHello %% %c%c%c%c%c%c%c: %d %i %u %o %x %X _start=%p main=%p fib=%p this-str=%p unknown pattern type: %q.\n"
hello_arg:
        pointer hello_str
        .ascii "numbers"
        .int 20
        .int -30
        .int -30
        .int 0100 # 64 in octal
        .int 0xAA55
        .int -1
        pointer _start
        pointer main
        pointer fib
        pointer hello_fmt
fib_fmt:
        .string "fib(%d) = %d\n"
