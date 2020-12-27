.include "machine-word.inc"
.align 2
.ifdef UART
.equ UART_BASE,         UART
.else
.equ UART_BASE,         0x10010000
.endif
.ifndef UART_BASE
MACHINE NOT SUPPORTED!
.endif

.equ UART_REG_TXFIFO,   0

.section .text
.global printf                          # C-like print formatted string. Supports formatting: %s, %c, %d, %i, %u, %x, %o, %p
                                        # IN: a0 = address of NULL terminated formatted string,
                                        #     a1 = address of argmuments

.global prints                          # Print string.
                                        # IN: a0 = address of NULL terminated string

.global printd                          # Print signed decimal number.
                                        # IN: a0 = decimal number

.global printu                          # Print unsigned decimal number.
                                        # IN: a0 = decimal number

.global printc                          # Print single character.
                                        # IN: a0 = char

.macro  push_printf_state
        stackalloc_x 3
        sx      ra, 0,(sp)
        sx      a0, 1,(sp)
        sx      a1, 2,(sp)
.endm
.macro  pop_printf_state
        lx      ra, 0,(sp)
        lx      a0, 1,(sp)
        lx      a1, 2,(sp)
        stackfree_x 3
.endm
.macro  call_printf_arg_handler label, load_op32, load_op64, size
        push_printf_state               # push state
        .ifdef RISCV32
        \load_op32 a0, 0(a1)
        .else
        \load_op64 a0, 0(a1)
        .endif
        jal     \label                  # invoke handler subroutine
        pop_printf_state                # pop state
        addi    a1, a1, \size           # advance arg pointer
.endm

printf:
0:      lbu     t0, (a0)                # load and zero-extend byte from address a0
        bnez    t0, 1f
        ret                             # while not null

1:      li      t1, '%'                 # handle %
        beq     t0, t1, 10f
2:      li      t2, UART_BASE
3:      lw      t1, UART_REG_TXFIFO(t2) # read from serial
        bltz    t1, 3b                  # until >= 0
        sw      t0, UART_REG_TXFIFO(t2) # write to serial
        addi    a0, a0, 1               # increment a6
        j       0b                      # continue

10:     addi    a0, a0, 2
        lbu     t0, -1(a0)              # read next character to determine type field (%s, %c, %d, %i, %u, %x, %o, %p)

        li      t1, '%'                 # if %%
        bne     t0, t1, 10f
        addi    a0, a0, -1              # print % and don't skip the next character
        j       2b

10:     li      t1, 'c'                 # if %c
        bne     t0, t1, 10f
                                        # load char from a1
                                        # a1 += sizeof(char)
                                        # print char
        call_printf_arg_handler printc, load_op32=lbu, load_op64=lbu, size=1
        j       0b                      # continue

10:     li      t1, 's'                 # if %s
        bne     t0, t1, 10f
                                        # load string pointer from a1
                                        # a1 += sizeof(char*)
                                        # print null-terminated string
        call_printf_arg_handler prints, load_op32=lw, load_op64=ld, size=REGBYTES
        j       0b                      # continue

10:     li      t1, 'd'                 # if %d
        beq     t0, t1, 1f
        li      t1, 'i'                 # or %i
        bne     t0, t1, 10f
1:                                      # load int from a1
                                        # a1 += sizeof(int)
                                        # print int
        call_printf_arg_handler printd, load_op32=lw, load_op64=lw, size=4
        j       0b                      # continue

10:     li      t1, 'u'                 # if %u
        bne     t0, t1, 10f
                                        # load unsigned int from a1
                                        # a1 += sizeof(unsigned)
                                        # print unsigned
        call_printf_arg_handler printu, load_op32=lw, load_op64=lwu, size=4
        j       0b                      # continue

10:     li      t1, 'o'                 # if %o
        bne     t0, t1, 10f
                                        # load unsigned int from a1
                                        # a1 += sizeof(unsigned)
                                        # print unsigned as octal
        call_printf_arg_handler printo, load_op32=lw, load_op64=lwu, size=4
        j       0b                      # continue

10:     li      t1, 'x'                 # if %x
        beq     t0, t1, 1f
        li      t1, 'X'                 # or %X
        bne     t0, t1, 10f
1:                                      # load unsigned int from a1
                                        # a1 += sizeof(unsigned)
                                        # print unsigned as hexadecimal
        call_printf_arg_handler printx, load_op32=lw, load_op64=lwu, size=4
        j       0b                      # continue

10:     li      t1, 'p'                 # if %p
        bne     t0, t1, 10f

        push_printf_state
        li      a0, '0'
        jal     printc
        li      a0, 'x'
        jal     printc                  # print '0x' in front of the address
        pop_printf_state
                                        # load void* pointer from a1
                                        # a1 += sizeof(void*)
                                        # print pointer address as hexadecimal
        call_printf_arg_handler printx, load_op32=lw, load_op64=ld, size=REGBYTES
        j       0b                      # continue

10:     addi    a0, a0, -2              # unknown argument
        lbu     t0, (a0)                # go back and print as characters
        j       2b

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
                                        # then print the negative sign first
        bgez    a0, printu
        neg     a0, a0                  # take two's complement of the input
        stackalloc_x 2
        sx      ra, 0,(sp)
        sx      a0, 1,(sp)
        li      a0, '-'
        jal     printc                  # print '-' in front of the rest
        lx      ra, 0,(sp)
        lx      a0, 1,(sp)
        stackfree_x 2

printu:
        li      a1, 10                  # radix = 10

print_radix:
        mv      a2, sp                  # store string on stack
        addi    sp, sp, -16             # allocate 16 symbols on stack to be safe
        addi    a2, a2, -1
        sb      zero, 0(a2)             # null-terminate the string

                                        # convert integer into the sequence of single digits
                                        # and push them onto stack in the reversed order
1:      remu    t0, a0, a1              # modulo radix
        li      t1, 10
        blt     t0, t1, 2f              # if t0 > 9
        addi    t0, t0, 'A'-'0'-10      #     t0 += 'A' - 10
2:      addi    t0, t0, '0'             # else t0 += '0'
        addi    a2, a2, -1
        sb      t0, 0(a2)
        divu    a0, a0, a1
        bnez    a0, 1b

        # print top of the stack
        mv      a0, a2
        addi    sp, sp, 16              # restore stack pointer (TODO: restore stack pointer after prints not before)
        j       prints

printo:
        li      a1, 8
        j       print_radix
printx:
        li      a1, 16
        j       print_radix
