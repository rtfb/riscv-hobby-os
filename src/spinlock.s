# This spinlock implementation is taken from RISC-V User-Level ISA v2.2,
# chapter 7.3 Atomic Memory Operations, fig 7.2.

.balign 4
.section .text
.globl acquire
acquire:
  li     t0, 1                  # store swap value in t0
again:
  amoswap.w.aq  t0, t0, (a0)    # attempt to acquire a lock
  bnez          t0, again       # try again if did not succeed
  ret


.globl release
release:
  amoswap.w.rl  x0, x0, (a0)    # Release the lock. When swapping with x0, it
                                # stores 0 and discards what's in (a0)
  ret
