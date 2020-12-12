
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
`make linux`
`make busybox`
`make initrd`
`make run`

[riscv-qemu-docs]: https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-qemu.html
[custom-kernel-tutorial]: http://mgalgs.github.io/2015/05/16/how-to-build-a-custom-linux-kernel-for-qemu-2015-edition.html

