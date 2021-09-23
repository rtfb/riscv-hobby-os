Run a RISC-V code in QEMU.

Bare metal RISC-V assembly in QEMU
==================================

Run a bare metal RISC-V code in QEMU without any OS or C. Based on the source
code from [here][riscv-hello-asm] and [here][riscv-hello-asm2].

This code is compiled with the [riscv-gnu-toolchain][riscv-gnu-toolchain] and
can be run with the QEMU `sifive_u` and `sifive_e` machines. Both 32bit and
64bit targets are supported.

As of this writing, these are the latest versions of the software involved:
* Qemu: `v5.1.0`
* RISC-V GNU toolchain: `10.1.0`

Make targets
------------

`make run-baremetal` -- build payload file and boot it via QEMU.

Debugging with gdb
------------------

1. `make download-sifive-toolchain` to download SiFive version of toolchain, as
   the regular apt-gettable toolchain won't work
2. Run as usual with and extra `DBG=1` argument, e.g. `make run-baremetal DBG=1`
3. In a separate terminal, run `make gdb`

RISC-V Linux in QEMU
====================

Run a RISC-V Linux system in Qemu. Based on instructions provided
[here][riscv-qemu-docs] and [here][custom-kernel-tutorial].

Documents the steps to build a Qemu locally, a custom Linux kernel, and Busybox
tools, as well as an initrd image.

As of this writing, these are the latest versions of the software involved:
* Qemu: `v5.1.0`
* Linux: `v5.9`
* Busybox: `1_9_2`
* RISC-V GNU toolchain: `10.1.0`

Make targets
------------

`make prereqs` -- apt-get the prerequisites.
`make clone` -- clone the relevant source code.
`make qemu` -- build Qemu for RISC-V.

`make run-linux` -- build Linux bootable image, add our executable and launch
via QEMU.

RISC-V ELF executable in SPIKE
==============================

Run a RISC-V ELF executable in [SPIKE][spike] a standard RISC-V ISA simulator.

As of this writing, these are the latest versions of the software involved:
* Qemu: `v5.1.0`
* Spike" `v1.0.1-dev`
* RISC-V GNU toolchain: `10.1.0`

TODO: install Spike

Make targets
------------

`make run-spike` -- build RISC-V ELF executable and launch it via SPIKE.

RISC-V Quick References
=======================

* [RISC-V Assembly Cheat Sheet][riscv-asm-sheet]
* [RISC-V Assembly Programmer's Manual][riscv-asm-man]
* [SiFive U (SiFive Freedom U540-C000 SoC) Hardware Manual][sifive-u]
* [SiFive E (SiFive Freedom U310-G002 SoC) Hardware Manual][sifive-e]

[riscv-gnu-toolchain]: https://github.com/riscv/riscv-gnu-toolchain
[riscv-qemu-docs]: https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-qemu.html
[custom-kernel-tutorial]: http://mgalgs.github.io/2015/05/16/how-to-build-a-custom-linux-kernel-for-qemu-2015-edition.html
[riscv-hello-asm]: https://github.com/noteed/riscv-hello-asm
[riscv-hello-asm2]: https://theintobooks.wordpress.com/2019/12/28/hello-world-on-risc-v-with-qemu
[spike]: https://github.com/riscv/riscv-isa-sim

[riscv-asm-sheet]: https://github.com/jameslzhu/riscv-card/blob/master/riscv-card.pdf
[riscv-asm-man]: https://github.com/riscv/riscv-asm-manual/blob/master/riscv-asm.md
[sifive-u]: https://static.dev.sifive.com/FU540-C000-v1.0.pdf
[sifive-e]: https://sifive.cdn.prismic.io/sifive%2F59a1f74e-d918-41c5-b837-3fe01ba7eaa1_fe310-g002-manual-v19p05.pdf
[sifive-toolchain]: https://www.sifive.com/software
