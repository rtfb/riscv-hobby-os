.include "src/machine-word.inc"
.balign 4
.section .text

.globl k_interrupt_timer
k_interrupt_timer:
        # swap t6 and mscratch. t6 now points to trap_frame and the
        # actual value of t6 is saved in mscratch until we can restore it a bit
        # later:
        csrrw   t6, mscratch, t6

        # save all user registers in trap_frame:
        sx       x1,  0, (t6)
        sx       x2,  1, (t6)
        sx       x3,  2, (t6)
        sx       x4,  3, (t6)
        sx       x5,  4, (t6)
        sx       x6,  5, (t6)
        sx       x7,  6, (t6)
        sx       x8,  7, (t6)
        sx       x9,  8, (t6)
        sx      x10,  9, (t6)
        sx      x11, 10, (t6)
        sx      x12, 11, (t6)
        sx      x13, 12, (t6)
        sx      x14, 13, (t6)
        sx      x15, 14, (t6)
        sx      x16, 15, (t6)
        sx      x17, 16, (t6)
        sx      x18, 17, (t6)
        sx      x19, 18, (t6)
        sx      x20, 19, (t6)
        sx      x21, 20, (t6)
        sx      x22, 21, (t6)
        sx      x23, 22, (t6)
        sx      x24, 23, (t6)
        sx      x25, 24, (t6)
        sx      x26, 25, (t6)
        sx      x27, 26, (t6)
        sx      x28, 27, (t6)
        sx      x29, 28, (t6)
        sx      x30, 29, (t6)

        # x31 is the same as t6, so store it below with a bit of juggling:
        mv      a0, t6
        csrrw   t6, mscratch, t6
        sx      t6, 30, (a0)

        # set kernel stack pointer:
        la      t0, stack_top           # set it at stack_top for hart0,
        csrr    t1, mhartid             # at stack_top+512 for hart1, etc.
        li      t2, 512
        mul     t1, t1, t2
        sub     t0, t0, t1
        mv      sp, t0

        # kernel_timer_tick will run the scheduler and will have populated
        # trap_frame with the context of the target user process.
        # ret_to_user will restore the registers from it.
        call    kernel_timer_tick

        # This will restore user registers from trap_frame and then mret:
        j       ret_to_user

.globl ret_to_user
ret_to_user:
        # load a pointer to trap_frame into t6:
        la      t6, trap_frame
        # now restore user's t6 to t6:
        lx      t6, 30, (t6)
        # and now the trick: swap user's t6 with mscratch, which also contains the
        # address of trap_frame. Now t6 is the pointer again and mscratch
        # preserves the value of user t6 until we can swap them back:
        csrrw   t6, mscratch, t6

        # now we're ready to restore all registers from trap_frame:
        lx       x1,  0, (t6)
        lx       x2,  1, (t6)
        lx       x3,  2, (t6)
        lx       x4,  3, (t6)
        lx       x5,  4, (t6)
        lx       x6,  5, (t6)
        lx       x7,  6, (t6)
        lx       x8,  7, (t6)
        lx       x9,  8, (t6)
        lx      x10,  9, (t6)
        lx      x11, 10, (t6)
        lx      x12, 11, (t6)
        lx      x13, 12, (t6)
        lx      x14, 13, (t6)
        lx      x15, 14, (t6)
        lx      x16, 15, (t6)
        lx      x17, 16, (t6)
        lx      x18, 17, (t6)
        lx      x19, 18, (t6)
        lx      x20, 19, (t6)
        lx      x21, 20, (t6)
        lx      x22, 21, (t6)
        lx      x23, 22, (t6)
        lx      x24, 23, (t6)
        lx      x25, 24, (t6)
        lx      x26, 25, (t6)
        lx      x27, 26, (t6)
        lx      x28, 27, (t6)
        lx      x29, 28, (t6)
        lx      x30, 29, (t6)

        # x31 is the same as t6, restore it from mscratch, and mscratch will
        # again preserve trap_frame for the next interrupt:
        csrrw   t6, mscratch, t6

        # return to userland:
        mret
