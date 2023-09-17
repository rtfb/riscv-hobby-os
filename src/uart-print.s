.include "src/machine-word.inc"
.balign 4

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
