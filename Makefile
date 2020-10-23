
run:
	sudo qemu-system-riscv64 -nographic -machine virt \
     -kernel linux/arch/riscv/boot/Image -append "root=/dev/vda ro console=ttyS0" \
     -drive file=busybox/busybox,format=raw,id=hd0 \
     -device virtio-blk-device,drive=hd0 \
     -initrd initramfs-busybox-riscv64.cpio.gz

prereqs:
	sudo apt install autoconf automake autotools-dev curl libmpc-dev \
	libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf \
	libtool patchutils bc zlib1g-dev libexpat-dev git

clone:
	git clone https://github.com/qemu/qemu
	git clone https://github.com/torvalds/linux
	git clone https://git.busybox.net/busybox

.PHONY: qemu
qemu:
	./scripts/build-qemu.sh

.PHONY: install-qemu
install-qemu:
	./scripts/install-qemu.sh

.PHONY: busybox
busybox:
	./scripts/build-busybox.sh

.PHONY: linux
linux:
	./scripts/build-linux.sh

.PHONY: initrd
initrd:
	./scripts/build-initrd.sh
