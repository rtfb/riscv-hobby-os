#include "asm.h"
.balign 4
.section .text

.globl k_interrupt_timer
k_interrupt_timer:
        // kernel_timer_tick will run the scheduler and will have populated
        // trap_frame with the context of the target user process.
        // ret_to_user will restore the registers from it.
        call    kernel_timer_tick

        // we saved the wrong sp inside swtch (slightly too deep), so let's
        // save sp again, now that we're back at the same level of stack that
        // the interrupt has entered in:
        mv      a0, sp
        call    save_sp

        // This will restore user registers from trap_frame and then mret:
        j       ret_to_user

.globl k_interrupt_plic
k_interrupt_plic:
        call    kernel_plic_handler

        // This will restore user registers from trap_frame and then mret:
        j       ret_to_user

.globl ret_to_user
ret_to_user:
        // load a pointer to trap_frame into t6:
        la      t6, trap_frame

        OP_LOAD t0, 31*REGSZ(t6)
        csrw    REG_EPC, t0

        // now restore user's t6 to t6:
        OP_LOAD t6, 30*REGSZ(t6)
        // and now the trick: swap user's t6 with mscratch, which also contains the
        // address of trap_frame. Now t6 is the pointer again and mscratch
        // preserves the value of user t6 until we can swap them back:
        csrrw   t6, REG_SCRATCH, t6

        // now we're ready to restore all registers from trap_frame:
        OP_LOAD  x1,  0*REGSZ(t6)
        OP_LOAD  x2,  1*REGSZ(t6)
        OP_LOAD  x3,  2*REGSZ(t6)
        OP_LOAD  x4,  3*REGSZ(t6)
        OP_LOAD  x5,  4*REGSZ(t6)
        OP_LOAD  x6,  5*REGSZ(t6)
        OP_LOAD  x7,  6*REGSZ(t6)
        OP_LOAD  x8,  7*REGSZ(t6)
        OP_LOAD  x9,  8*REGSZ(t6)
        OP_LOAD x10,  9*REGSZ(t6)
        OP_LOAD x11, 10*REGSZ(t6)
        OP_LOAD x12, 11*REGSZ(t6)
        OP_LOAD x13, 12*REGSZ(t6)
        OP_LOAD x14, 13*REGSZ(t6)
        OP_LOAD x15, 14*REGSZ(t6)
        OP_LOAD x16, 15*REGSZ(t6)
        OP_LOAD x17, 16*REGSZ(t6)
        OP_LOAD x18, 17*REGSZ(t6)
        OP_LOAD x19, 18*REGSZ(t6)
        OP_LOAD x20, 19*REGSZ(t6)
        OP_LOAD x21, 20*REGSZ(t6)
        OP_LOAD x22, 21*REGSZ(t6)
        OP_LOAD x23, 22*REGSZ(t6)
        OP_LOAD x24, 23*REGSZ(t6)
        OP_LOAD x25, 24*REGSZ(t6)
        OP_LOAD x26, 25*REGSZ(t6)
        OP_LOAD x27, 26*REGSZ(t6)
        OP_LOAD x28, 27*REGSZ(t6)
        OP_LOAD x29, 28*REGSZ(t6)
        OP_LOAD x30, 29*REGSZ(t6)

        // x31 is the same as t6, restore it from mscratch, and mscratch will
        // again preserve trap_frame for the next interrupt:
        csrrw   t6, REG_SCRATCH, t6

        // return to userland:
        OP_xRET

// Context switch
//
//   void swtch(context_t *old, context_t *new);
//
// Save ra, sp and all sXX registers in old context, restore the same registers
// from new, then 'ret'. Since this changes ra, this will effectively return
// into another execution context, something that was saved previously in old.
.globl swtch
swtch:
        OP_STOR ra,   0*REGSZ(a0)
        OP_STOR sp,   1*REGSZ(a0)
        OP_STOR s0,   2*REGSZ(a0)
        OP_STOR s1,   3*REGSZ(a0)
        OP_STOR s2,   4*REGSZ(a0)
        OP_STOR s3,   5*REGSZ(a0)
        OP_STOR s4,   6*REGSZ(a0)
        OP_STOR s5,   7*REGSZ(a0)
        OP_STOR s6,   8*REGSZ(a0)
        OP_STOR s7,   9*REGSZ(a0)
        OP_STOR s8,  10*REGSZ(a0)
        OP_STOR s9,  11*REGSZ(a0)
        OP_STOR s10, 12*REGSZ(a0)
        OP_STOR s11, 13*REGSZ(a0)

        OP_LOAD ra,   0*REGSZ(a1)
        OP_LOAD sp,   1*REGSZ(a1)
        OP_LOAD s0,   2*REGSZ(a1)
        OP_LOAD s1,   3*REGSZ(a1)
        OP_LOAD s2,   4*REGSZ(a1)
        OP_LOAD s3,   5*REGSZ(a1)
        OP_LOAD s4,   6*REGSZ(a1)
        OP_LOAD s5,   7*REGSZ(a1)
        OP_LOAD s6,   8*REGSZ(a1)
        OP_LOAD s7,   9*REGSZ(a1)
        OP_LOAD s8,  10*REGSZ(a1)
        OP_LOAD s9,  11*REGSZ(a1)
        OP_LOAD s10, 12*REGSZ(a1)
        OP_LOAD s11, 13*REGSZ(a1)

        ret
