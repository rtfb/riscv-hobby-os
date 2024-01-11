Timer Implementation
====================

## Interaction with the timer

The current time is mostly accessed via a memory-mapped register `MTIME`,
declared in [`timer.h`][timer-h]. There are a couple exceptions: on Ox64 and on
D1, the current time is being read from `time` CSR, which "is a read-only shadow
of the memory-mapped mtime register", according to the spec.

The time comparator is also mostly accessed via a memory-mapped register
`MTIMECMP_BASE`, declared in [`timer.h`][timer-h]. The same two machines are
exceptional as well.

On D1, the comparator has a different address, and that address is only
accessible in 32-bit chunks (see [`d1/timer.c`][timer-c-d1]).

On Ox64 there doesn't seem to be a way to access a timer comparator directly, so
it's accessed via an SBI ecall (see [`ox64/timer.c`][timer-c-ox64]).

The timer frequencies differ quite a lot from machine to machine, and there
doesn't seem to be a way to detect or derive the frequency. Thus, each
machine-specific header file at [`include/machine/`][include-machine] declares a
`ONE_SECOND` constant, which specifies the number of `mtime` ticks in a second.

## Timer interrupt handling

There are two timer interrupt handling codepaths that introduce some subtleties.
The simpler one works on machines where we operate in M+U or S+U modes. However,
on machines where we operate in M+S+U modes things are more complicated.

#### M+U and S+U modes

In these simpler setups, the timer interrupt always arrives at the
[`trap_vector`][boot-s] and eventually reaches [`interrupt_vector`][boot-s],
where both supervisor and machine mode sources jump to the actual handler,
[`k_interrupt_timer`][context-s].

#### M+S+U modes

When the kernel is in charge of handling both the M and S modes, the timer
interrupt always arrives in M mode, even when trap delegation is configured via
`mideleg` and `medeleg` CSRs. (This differs from the way how other interrupts
behave: with delegation configured, external and software interrupts arrive
directly in S mode). Thus, the timer interrupt needs handling both in M and S
modes and that is indicated by a [`MIXED_MODE_TIMER`][timer-h] constant.

The first difference of mixed mode timer is the entry point of the timer
interrupt. It is handled in [`mtimertrap`][boot-s], which does two things:
1. increments the time comparator for the next timer tick
2. issues a software interrupt into S mode for the actual handing

It is important to increment the time comparator at this point because according
to the spec, "`MTIP` is read-only in `mip`, and is cleared by writing to the
memory-mapped machine-mode timer compare register". So if we don't increment the
comparator, the timer interrupt would immediately retrigger when we call `mret`
to switch to S mode.

The software interrupt is issued by manually setting the `mip.SSIP` bit and
calling `mret`, which will get trapped by `trap_vector` and from this point the
handling is very similar to the non-mixed mode.

A couple differences remain. Because the next timer tick was scheduled in
`mtimertrap`, all other calls to `set_timer_after()` are ifdef'ed to only be
called in the non-mixed mode. Also, since we enter the handler via a software
interrupt, we need to manually clear the `sip.SSIP` (software interrupt pending)
bit before leaving the timer handler (for the same reason as comparator
increment above - to avoid an immediate retrigger).

`mtimertrap` depends on additional initialization, which is performed in
[`init_timer`][timer-c].

## Discussion

#### `SSIP` vs `STIP`

It may seem strange that a _software_ interrupt is being used to delegate a
timer interrupt to S mode instead of an _S-Mode timer_ interrupt. After all, the
spec clearly suggests that:

"If supervisor mode is implemented, bits `mip.STIP` and `mie.STIE` are the
interrupt-pending and interrupt-enable bits for supervisor-level timer
interrupts. `STIP` is writable in `mip`, and may be written by M-mode software
to deliver timer interrupts to S-mode."

However, just like M mode can't clear `mip.MTIP`, S mode can't clear `sip.STIP`,
and that's confirmed in the spec as well:

"If implemented, `STIP` is read-only in `sip`, and is set and cleared by the
execution environment."

So using the S mode timer interrupt would require another mode transition just
to clear the `sip.STIP` bit, which would be both suboptimal, and further bloat
the implementation.

This idea of using the software interrupt instead of timer interrupt was taken
from [xv6][xv6-timervec], but both experiments and the spec confirm that this is
the only possible way to implement mixed mode timer interrupt handling without
resorting to a full-featured M mode SBI implementation.

#### Sstc

A better alternative for handling timer in S mode is being drafted in the
[Sstc][sstc] extension. The extension introduces a new CSR `stimecmp`, which is
read-writable in S-mode and will simplify timer interrupt scheduling and
handling.

Qemu already implements this extension, so this can already be used. However, my
GCC toolchain currently does not implement it - a problem that can be worked
around by accessing `stimecmp` by its register number. A bigger (potential)
problem is compatibility: on machines where Sstc is not yet implemented, we'd
still have to resort to a mixed mode timer, so that would add yet another
codepath.

[boot-s]: https://github.com/rtfb/riscv-hobby-os/tree/master/src/boot.S
[context-s]: https://github.com/rtfb/riscv-hobby-os/tree/master/src/context.S
[include-machine]: https://github.com/rtfb/riscv-hobby-os/tree/master/include/machine/
[sstc]: https://github.com/riscv/riscv-time-compare
[timer-c]: https://github.com/rtfb/riscv-hobby-os/tree/master/src/timer.c
[timer-c-d1]: https://github.com/rtfb/riscv-hobby-os/tree/master/src/machine/d1/timer.c
[timer-c-ox64]: https://github.com/rtfb/riscv-hobby-os/tree/master/src/machine/ox64/timer.c
[timer-h]: https://github.com/rtfb/riscv-hobby-os/tree/master/include/timer.h
[xv6-timervec]: https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/kernelvec.S#L95-L124
