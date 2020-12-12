
.global returner
returner:
    # Prologue
    addi    sp, sp, -32
    sd      ra, 0(sp)
    sd      a0, 8(sp)
    sd      s0, 16(sp)
    sd      s1, 24(sp)

	li a0, 13

    # Epilogue
    ld      ra, 0(sp)
    ld      a0, 8(sp)
    ld      s0, 16(sp)
    ld      s1, 24(sp)
    addi    sp, sp, 32
    ret



.section .rodata
prompt: .asciz "Value of t0 = %ld and value of t1 = %ld\n"
.section .text
.global asmprinter
asmprinter:
    addi    sp, sp, -8
    sd      ra, 0(sp)

	li t0, 7
	li t1, 19

    la      a0, prompt
    mv      a1, t0
    mv      a2, t1
    call    printf
    ld      ra, 0(sp)
    addi    sp, sp, 8
    ret
