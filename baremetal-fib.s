.align 2
.equ UART_BASE,         0x10010000
.equ UART_REG_TXFIFO,   0

.section .text
.globl _start

_start:
        csrr    a1, mhartid             # read our hartware thread id (`hart` stands for `hardware thread`)
        bnez    a1, halt                # run only on one hardware thread (hardid == 0), halt all the other ones

        la      sp, stack_top           # setup stack pointer

        jal     main                    # call main()

halt:   j       halt

.global main
main:
        la      a0, hello_msg
        jal     prints

        # test printd and prints functions
        li      a0, 1
        jal     printd
        li      a0, 5
        jal     printd
        li      a0, 10
        jal     printd
        li      a0, 100
        jal     printd
        li      a0, 130
        jal     printd
        li      a0, -130
        jal     printd
        li      a0, -255
        jal     printd
        la      a0, next_line
        jal     prints

        # calculate Fibonacci sequence
        # for (int i = 1; i < 15; i++)
        li      s0, 1
        li      s1, 15
        # printf("fib(%d) = %d\n", i, fib(i));
1:      la      a0, fib0_msg
        jal     prints                    # print "fib("
        mv      a0, s0
        jal     printd                    # print "%d"
        mv      a0, s0
        jal     fib                       # call fib()
        mv      s2, a0
        la      a0, fib1_msg
        jal     prints                    # print ") ="
        mv      a0, s2
        jal     printd                    # print "%d"
        la      a0, next_line
        jal     prints                    # print "\n"
        addi    s0, s0, 1
        blt     s0, s1, 1b

        ret

.global fib
fib:
        li      t0, 1
        li      t1, 2

        beq     a0, t0, 1f
        beq     a0, t1, 1f

        mv      t0, a0
        addi    a0, t0, -1              # calculate n-1

        addi    sp, sp, -16
        sd      ra, 0(sp)
        sd      t0, 8(sp)               # preserve t0, which contains our original argument
        jal     fib
        ld      ra, 0(sp)
        ld      t0, 8(sp)
        addi    sp, sp, 16

        mv      t2, a0                  # t2 now contains fib(n-1)

        addi    a0, t0, -2              # calculate n-2

        addi    sp, sp, -16
        sd      ra, 0(sp)
        sd      t2, 8(sp)               # preserve t2, which has fib(n-1)
        jal     fib
        ld      ra, 0(sp)
        ld      t2, 8(sp)
        addi    sp, sp, 16

        mv      t3, a0                  # t3 now contains fib(n-2)
        add     a0, t2, t3              # add them and jump to return
        j       2f

1:
        li      a0, 1

2:
        ret

.section .rodata
hello_msg:
        .string "Hello world!\n"
fib0_msg:
        .string "fib("
fib1_msg:
        .string ") = "
next_line:
        .string "\n"

# --- Utility functions ------------------------------------------------------------

.section .text
.global printc
printc:                                 # IN: a0 = char
        li      a1, UART_BASE
1:      lw      t1, UART_REG_TXFIFO(a1) # read from serial
        bltz    t1, 1b                  # until >= 0
        sw      a0, UART_REG_TXFIFO(a1) # write to serial
        ret

.global prints
prints:                                 # IN: a0 = address of NULL terminated string
        li      a1, UART_BASE
1:      lbu     t0, (a0)                # load and zero-extend byte from address a0
        beqz    t0, 3f                  # while not null
2:      lw      t1, UART_REG_TXFIFO(a1) # read from serial
        bltz    t1, 2b                  # until >= 0
        sw      t0, UART_REG_TXFIFO(a1) # write to serial
        addi    a0, a0, 1               # increment a0
        j       1b
3:      ret

.global printd
printd:                                 # IN: a0 = decimal number
        # if input is negative,
        # then print negative sign
        bgez    a0, 1f
        neg     a0, a0                  # take two's complement of the input
        addi    sp, sp, -16
        sd      ra, 0(sp)
        sd      a0, 8(sp)
        li      a0, '-'
        jal     printc
        ld      ra, 0(sp)
        ld      a0, 8(sp)
        addi    sp, sp, 16

1:      li      a1, 10                  # radix = 10
        mv      a2, sp                  # store string on stack
        addi    sp, sp, -1
        sb      zero, 0(sp)             # null-terminate the string

        # convert integer into the
        # sequence of single digits
        # and push them onto stack
2:      rem     t0, a0, a1                # modulo radix
        addi    t0, t0, '0'
        addi    sp, sp, -1
        sb      t0, 0(sp)
        div     a0, a0, a1
        bnez    a0, 2b

        # print top of the stack
        mv      a0, sp
        mv      sp, a2
        j       prints
