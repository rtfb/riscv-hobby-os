.include "src/machine-word.inc"
.balign 4
.section .text

.globl k_interrupt_timer
k_interrupt_timer:
        # kernel_timer_tick will run the scheduler and will have populated
        # trap_frame with the context of the target user process.
        # ret_to_user will restore the registers from it.
        call    kernel_timer_tick

        # we saved the wrong sp inside swtch (slightly too deep), so let's
        # save sp again, now that we're back at the same level of stack that
        # the interrupt has entered in:
        mv      a0, sp
        call    save_sp

        # This will restore user registers from trap_frame and then mret:
        j       ret_to_user

.globl k_interrupt_plic
k_interrupt_plic:
        call    kernel_plic_handler

        # This will restore user registers from trap_frame and then mret:
        j       ret_to_user

.globl ret_to_user
ret_to_user:
        # load a pointer to trap_frame into t6:
        la      t6, trap_frame

        lx      t0, 31, (t6)
        csrw    mepc, t0

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

        # TODO: shouldn't we call set_user_mode here? We now only call it from
        # schedule_user_process, which means that we often return to userland
        # while the hart is in M-mode.

        # return to userland:
        mret

# Context switch
#
#   void swtch(context_t *old, context_t *new);
#
# Save ra, sp and all sXX registers in old context, restore the same registers
# from new, then 'ret'. Since this changes ra, this will effectively return
# into another execution context, something that was saved previously in old.
.globl swtch
swtch:
        sx      ra,  0,  (a0)
        sx      sp,  1,  (a0)
        sx      s0,  2,  (a0)
        sx      s1,  3,  (a0)
        sx      s2,  4,  (a0)
        sx      s3,  5,  (a0)
        sx      s4,  6,  (a0)
        sx      s5,  7,  (a0)
        sx      s6,  8,  (a0)
        sx      s7,  9,  (a0)
        sx      s8,  10, (a0)
        sx      s9,  11, (a0)
        sx      s10, 12, (a0)
        sx      s11, 13, (a0)

        lx      ra,  0,  (a1)
        lx      sp,  1,  (a1)
        lx      s0,  2,  (a1)
        lx      s1,  3,  (a1)
        lx      s2,  4,  (a1)
        lx      s3,  5,  (a1)
        lx      s4,  6,  (a1)
        lx      s5,  7,  (a1)
        lx      s6,  8,  (a1)
        lx      s7,  9,  (a1)
        lx      s8,  10, (a1)
        lx      s9,  11, (a1)
        lx      s10, 12, (a1)
        lx      s11, 13, (a1)

        ret
