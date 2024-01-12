# RISC-V Hobby OS

This is a small hobbyist kernel for RISC-V. It doesn't try to be compatible with
anything, but happens to go along the old Unix ways. It does preemptive
multitasking, implements fork+exec system calls, and a bunch of others needed
for implementation of a simple shell. It also implements pipes.

It runs in qemu, as well as several physical RISC-V SBC boards:
* [SiFive HiFive1 Rev B][hifive-url]
* [Ox64][ox64-url]
* [Nezha D1][d1-url]
* [Pine64 Star64][star64-url]

(Note: the 64-bit qemu sifive_e target is known to be broken; it works, as such,
but uses up too much of the 16kB of available RAM, with not enough remaining to
run even a single process)

Building And Running
====================

On Ubuntu, `make prereqs` will install the prerequisite software for building.

`make build-qemu-image` will build a Docker image for running qemu. This
simplifies running qemu on different versions of Ubuntu and on OSX.

With that done, you should be able to `make all` to build all targets, and then
`make run-virt` to actually run it in qemu.

Implementation Details
======================

The kernel and userland programs are built into a single binary. So the userland
programs are actually just functions, located in a separate memory section. The
section is isolated from the kernel memory, though, via RISC-V PMP mechanism.

On some targets the kernel supports virtual memory, and in such case the
processes are better isolated. See [`docs/mmu.md`][docs-mmu] for details.

Some other targets do not implement virtual memory. This is because the smallest
common denominator of supported hardware is an extremely small HiFive
microcontroller, with only 16kB of RAM and no MMU. This puts severe limitations.
We want to maintain an ability to run in such small environments, as we intend
to use this kernel on a small implementation of a RISC-V core on a modestly
sized FPGA.

Debugging with gdb
------------------

#### Qemu

1. Obtain gdb ([`docs/gdb.md`][docs-gdb]).
2. Run as usual with an extra `DBG=1` argument, e.g. `make run-virt DBG=1`.
3. In a separate terminal, run `make gdb`. Gdb will connect to the above, and
   you can use it as usual.

More Docs
=========

The [`docs/`][docs-folder] folder contains a few documents on specific narrow
subjects, like operating a specific SBC board.

Handy RISC-V Quick References:
* [RISC-V Assembly Cheat Sheet][riscv-asm-sheet]
* [RISC-V Assembly Programmer's Manual][riscv-asm-man]
* [SiFive U (SiFive Freedom U540-C000 SoC) Hardware Manual][sifive-u]
* [SiFive E (SiFive Freedom U310-G002 SoC) Hardware Manual][sifive-e]

[riscv-asm-sheet]: https://github.com/jameslzhu/riscv-card/blob/master/riscv-card.pdf
[riscv-asm-man]: https://github.com/riscv/riscv-asm-manual/blob/master/riscv-asm.md
[sifive-u]: https://static.dev.sifive.com/FU540-C000-v1.0.pdf
[sifive-e]: https://sifive.cdn.prismic.io/sifive%2F59a1f74e-d918-41c5-b837-3fe01ba7eaa1_fe310-g002-manual-v19p05.pdf

[d1-url]: https://d1.docs.aw-ol.com/en/d1_dev/
[hifive-url]: https://www.sifive.com/boards/hifive1-rev-b
[ox64-url]: https://wiki.pine64.org/wiki/Ox64
[star64-url]: https://wiki.pine64.org/wiki/STAR64

[docs-folder]: https://github.com/rtfb/riscv-hobby-os/tree/master/docs
[docs-gdb]: https://github.com/rtfb/riscv-hobby-os/tree/master/docs/gdb.md
[docs-mmu]: https://github.com/rtfb/riscv-hobby-os/tree/master/docs/mmu.md
