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
        # test printf()
        la      a0, hello_fmt
        la      a1, hello_arg
        call    printf

        # calculate Fibonacci sequence
        # for (int i = 1; i < 15; i++)
        #     printf("fib(%d) = %d\n", i, fib(i));
        li      s0, 1
        li      s1, 15
1:      mv      a0, s0
        jal     fib                     # call fib() with a0 containing index of Fibonacci sequence.
                                        # The function will return result in a0.

        addi    sp, sp, -8              # allocate 2 printf arguments (size of int) on stack
        mv      a1, sp
        sd      s0, 0(a1)               # 1st printf argument s0 contains index i
        sd      a0, 4(a1)               # 2nd prinnt argument a0 contains result of fib(i)
        la      a0, fib_fmt
        call    printf                  # call printf with a0 pointing to "fib(%d) = %d\n" pattern
                                        #              and a1 pointing to list of arguments (i, fib(i))
        addi    sp, sp, 8              # restore stack

        addi    s0, s0, 1               # i++
        blt     s0, s1, 1b              # loop while i < 15

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
hello_str:
        .string "Hello world!!!\n"
hello_fmt:
        .string "%sHello %c%c%c%c%c%c%c: %d %i %u.\n"
hello_arg:
        .dword hello_str
        .ascii "numbers"
        .int 20
        .int -30
        .int -30
fib_fmt:
        .string "fib(%d) = %d\n"

# --- Utility functions ------------------------------------------------------------

.section .text
.global printf

printf:                                 # IN: a0 = address of NULL terminated formatted string, a1 = address of argmuments
                                        # formatting supports %s, %c, %d, %i, %u, TODO: %x, %o, %p
        lbu     t0, (a0)                # load and zero-extend byte from address a0
        bnez    t0, 1f
        ret                             # while not null

1:      li      t1, '%'                 # handle %
        beq     t0, t1, 10f
2:      li      t2, UART_BASE
3:      lw      t1, UART_REG_TXFIFO(t2) # read from serial
        bltz    t1, 3b                  # until >= 0
        sw      t0, UART_REG_TXFIFO(t2) # write to serial
        addi    a0, a0, 1               # increment a0
        j       printf                  # continue

10:     addi    a0, a0, 2
        lbu     t0, -1(a0)              # read next character to determine type field (%s, %c, %d, %i, %u, %x, %o, %p)

        li      t1, 'c'                 # if %c
        bne     t0, t1, 10f
        addi    sp, sp, -24
        sd      ra, 0(sp)
        sd      a0, 8(sp)
        sd      a1, 16(sp)
        lbu     a0, (a1)                # load character from address a1
        jal     printc
        ld      ra, 0(sp)
        ld      a0, 8(sp)
        ld      a1, 16(sp)
        addi    sp, sp, 24
        addi    a1, a1, 1               # increment a1
        j       printf                  # continue

10:     li      t1, 's'                 # if %s
        bne     t0, t1, 10f
        addi    sp, sp, -24
        sd      ra, 0(sp)
        sd      a0, 8(sp)
        sd      a1, 16(sp)
        ld      a0, (a1)                # load string pointer from address a1
        jal     prints
        ld      ra, 0(sp)
        ld      a0, 8(sp)
        ld      a1, 16(sp)
        addi    sp, sp, 24
        addi    a1, a1, 8               # increment a1
        j       printf                  # continue

10:     li      t1, 'd'                 # if %d
        beq     t0, t1, 11f
        li      t1, 'i'                 # if %i
        bne     t0, t1, 10f
11:     addi    sp, sp, -24
        sd      ra, 0(sp)
        sd      a0, 8(sp)
        sd      a1, 16(sp)
        lw      a0, (a1)                # load int from address a1
        jal     printd
        ld      ra, 0(sp)
        ld      a0, 8(sp)
        ld      a1, 16(sp)
        addi    sp, sp, 24
        addi    a1, a1, 4               # increment a1
        j       printf                  # continue

10:     li      t1, 'u'                 # if %u
        bne     t0, t1, 10f
        addi    sp, sp, -24
        sd      ra, 0(sp)
        sd      a0, 8(sp)
        sd      a1, 16(sp)
        lwu     a0, (a1)                # load unsigned int from address a1
        jal     printu
        ld      ra, 0(sp)
        ld      a0, 8(sp)
        ld      a1, 16(sp)
        addi    sp, sp, 24
        addi    a1, a1, 4               # increment a1
        j       printf                  # continue

10:     j       2b                      # default - print current character


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
.global printu
printd:                                 # IN: a0 = decimal number
        # if input is negative,
        # then print negative sign
        bgez    a0, printu
        neg     a0, a0                  # take two's complement of the input
        addi    sp, sp, -16
        sd      ra, 0(sp)
        sd      a0, 8(sp)
        li      a0, '-'
        jal     printc                  # print '-' in front of the rest
        ld      ra, 0(sp)
        ld      a0, 8(sp)
        addi    sp, sp, 16

printu:
        li      a1, 10                  # radix = 10
        mv      a2, sp                  # store string on stack
        addi    sp, sp, -16             # allocate 16 symbols on stack to be safe
        addi    a2, a2, -1
        sb      zero, 0(a2)             # null-terminate the string

        # convert integer into the
        # sequence of single digits
        # and push them onto stack
2:      remu    t0, a0, a1                # modulo radix
        addi    t0, t0, '0'
        addi    a2, a2, -1
        sb      t0, 0(a2)
        divu    a0, a0, a1
        bnez    a0, 2b

        # print top of the stack
        mv      a0, a2
        addi    sp, sp, 16              # restore stack pointer (TODO: restore stack pointer after prints not before)
        j       prints
