Run a RISC-V code in QEMU.

Bare metal RISC-V assembly in QEMU
=================================

Run a bare metal RISC-V code in QEMU without any OS or C. Based on the source code from [here][riscv-hello-asm] and [here][riscv-hello-asm2].

The code supports 64bit `sifive_u` QEMU emulation target.

As of this writing, these are the latest versions of the software involved:
* Qemu: `v5.1.0`

Make targets
------------

`make run-baremetal` -- build payload file and boot it via QEMU.

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

Make targets
------------

`make prereqs` -- apt-get the prerequisites.
`make clone` -- clone the relevant source code.
`make qemu` -- build Qemu for RISC-V.

`make run-linux` -- build Linux bootable image, add our executable and launch via QEMU.

RISC-V ELF executable in SPIKE
=================================

Run a RISC-V ELF executable in [SPIKE][spike] a standard RISC-V ISA simulator.

As of this writing, these are the latest versions of the software involved:
* Qemu: `v5.1.0`
* Spike" `v1.0.1-dev`

TODO: install Spike

`make run-spike` -- build RISC-V ELF executable and launch it via SPIKE.

[riscv-qemu-docs]: https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-qemu.html
[custom-kernel-tutorial]: http://mgalgs.github.io/2015/05/16/how-to-build-a-custom-linux-kernel-for-qemu-2015-edition.html
[riscv-hello-asm]: https://github.com/noteed/riscv-hello-asm
[riscv-hello-asm2]: https://theintobooks.wordpress.com/2019/12/28/hello-world-on-risc-v-with-qemu
[spike]: https://github.com/riscv/riscv-isa-sim

