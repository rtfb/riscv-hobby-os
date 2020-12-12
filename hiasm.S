
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


.global fib
fib:
	li      t0, 1
	li      t1, 2

	beq     a0, t0, 1f
	beq     a0, t1, 1f

	mv      t0, a0
	li      t1, 1
	sub     a0, t0, t1   # calculate n-1

	addi    sp, sp, -16
	sd      ra, 0(sp)
	sd      t0, 8(sp)    # preserve t0, which contains our original argument
	call    fib
	ld      ra, 0(sp)
	ld      t0, 8(sp)
	addi    sp, sp, 16

	mv      t2, a0       # t2 now contains fib(n-1)

	li      t1, 2
	sub     a0, t0, t1   # calculate n-2

	addi    sp, sp, -16
	sd      ra, 0(sp)
	sd      t2, 8(sp)    # preserve t2, which has fib(n-1)
	call    fib
	ld      ra, 0(sp)
	ld      t2, 8(sp)
	addi    sp, sp, 16

	mv      t3, a0       # t3 now contains fib(n-2)
	add     a0, t2, t3   # add them and jump to return
	j       2f

1:
	li      a0, 1

2:
	ret
