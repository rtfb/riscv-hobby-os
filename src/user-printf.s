.include "src/machine-word.inc"

# printf is a thin wrapper around printfvec, which does the actual formatted
# output. We need this wrapper to collect all a1..a7 registers onto stack
# without corrupting them. I couldn't do that in C, since gcc insists on using
# a5 as a temp register.
.section .user_text
.global printf
printf:
        stackalloc_x 8
        sx      a1, 0,(sp)
        sx      a2, 1,(sp)
        sx      a3, 2,(sp)
        sx      a4, 3,(sp)
        sx      a5, 4,(sp)
        sx      a6, 5,(sp)
        sx      a7, 6,(sp)
        sx      ra, 7,(sp)
        mv      a1, sp
        call    printfvec
        lx      ra, 7,(sp)
        stackfree_x 8
        ret
