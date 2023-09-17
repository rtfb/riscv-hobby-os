.include "src/machine-word.inc"
.balign 4
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
.global uart_printc                     # Print single character.
                                        # @param[in] a0 char

# kprintf is called with args in a0..a7, but kprintfvec expects to get fmt
# string in a0 and a pointer to the rest of args in a1. So push a1..a7 to stack
# and pass a pointer to that location in a1.
.globl kprintf
kprintf:
        stackalloc_x 8
        mv      t0, sp
        sx      a1, 0, (sp)
        sx      a2, 1, (sp)
        sx      a3, 2, (sp)
        sx      a4, 3, (sp)
        sx      a5, 4, (sp)
        sx      a6, 5, (sp)
        sx      a7, 6, (sp)
        sx      ra, 7, (sp)
        mv      a1, t0
        call    kprintfvec
        lx      ra, 7, (sp)
        stackfree_x 8
        ret

.global uart_printc
uart_printc:                            # @param[in] a0 char
        li      a1, UART_BASE
1:      lw      t1, UART_REG_TXFIFO(a1) # read from serial
        bltz    t1, 1b                  # until >= 0
        sw      a0, UART_REG_TXFIFO(a1) # write to serial
        ret

.global uart_prints
uart_prints:                            # @param[in] a0 address of NULL terminated string
        li      a1, UART_BASE
        mv      t2, a0                  # backup a0 for size calculation before ret
1:      lbu     t0, (a0)                # load and zero-extend byte from address a0
        beqz    t0, 3f                  # while not null
2:      lw      t1, UART_REG_TXFIFO(a1) # read from serial
        bltz    t1, 2b                  # until >= 0
        sw      t0, UART_REG_TXFIFO(a1) # write to serial
        addi    a0, a0, 1               # increment a0
        j       1b
3:      sub     a0, a0, t2              # return the number of bytes written
        ret
