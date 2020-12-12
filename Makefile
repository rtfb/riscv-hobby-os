
run:
	./qemu-build/bin/qemu-system-riscv64 -nographic -machine virt \
     -kernel linux/arch/riscv/boot/Image -append "root=/dev/vda ro console=ttyS0" \
     -drive file=busybox/busybox,format=raw,id=hd0 \
     -device virtio-blk-device,drive=hd0 \
     -initrd initramfs-busybox-riscv64.cpio.gz

prereqs:
	sudo apt --yes install \
		autoconf \
		automake \
		autotools-dev \
		bc \
		bison \
		build-essential \
		curl \
		flex \
		gawk \
		git \
		libexpat-dev \
		libgmp-dev \
		libmpc-dev \
		libmpfr-dev \
		libtool \
		patchutils \
		texinfo gperf \
		zlib1g-dev

clone:
	git clone https://github.com/qemu/qemu
	git clone https://github.com/torvalds/linux
	git clone https://git.busybox.net/busybox

.PHONY: qemu
qemu:
	./scripts/build-qemu.sh

.PHONY: busybox
busybox:
	./scripts/build-busybox.sh

.PHONY: linux
linux:
	./scripts/build-linux.sh

.PHONY: initrd
initrd:
	./scripts/build-initrd.sh
