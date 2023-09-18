#include "asm.h"
.balign 4

# kprintf is called with args in a0..a7, but kprintfvec expects to get fmt
# string in a0 and a pointer to the rest of args in a1. So push a1..a7 to stack
# and pass a pointer to that location in a1.
.globl kprintf
kprintf:
        addi    sp, sp, -8*REGSZ
        mv      t0, sp
        OP_STOR a1, 0*REGSZ(sp)
        OP_STOR a2, 1*REGSZ(sp)
        OP_STOR a3, 2*REGSZ(sp)
        OP_STOR a4, 3*REGSZ(sp)
        OP_STOR a5, 4*REGSZ(sp)
        OP_STOR a6, 5*REGSZ(sp)
        OP_STOR a7, 6*REGSZ(sp)
        OP_STOR ra, 7*REGSZ(sp)
        mv      a1, t0
        call    kprintfvec
        OP_LOAD ra, 7*REGSZ(sp)
        addi    sp, sp, 8*REGSZ
        ret
