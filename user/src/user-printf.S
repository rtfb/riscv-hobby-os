#include "asm.h"

# printf is a thin wrapper around printfvec, which does the actual formatted
# output. We need this wrapper to collect all a1..a7 registers onto stack
# without corrupting them. I couldn't do that in C, since gcc insists on using
# a5 as a temp register.
.section .user_text
.global printf
printf:
        addi    sp, sp, -8*REGSZ
        OP_STOR a1, 0*REGSZ(sp)
        OP_STOR a2, 1*REGSZ(sp)
        OP_STOR a3, 2*REGSZ(sp)
        OP_STOR a4, 3*REGSZ(sp)
        OP_STOR a5, 4*REGSZ(sp)
        OP_STOR a6, 5*REGSZ(sp)
        OP_STOR a7, 6*REGSZ(sp)
        OP_STOR ra, 7*REGSZ(sp)
        mv      a1, sp
        call    printfvec
        OP_LOAD ra, 7*REGSZ(sp)
        addi    sp, sp, 8*REGSZ
        ret
