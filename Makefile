# If local QEmu build does not exist, use system wide installed QEmu
QEMU ?= ./qemu-build/bin/qemu-system-riscv64
ifeq ($(wildcard $(QEMU)),)
	QEMU = qemu-system-riscv64
endif
# Spike, the RISC-V ISA Simulator (https://github.com/riscv/riscv-isa-sim)
SPIKE ?= spike

# Shortcuts
runm: run-baremetal
runb: run-baremetal
runs: run-spike
runl: run-linux

run-baremetal: baremetal
	$(QEMU) -nographic -machine sifive_u -bios none -kernel baremetal/fib

run-spike: elf
	$(SPIKE) pk generic-elf/hello bbl loader

run-linux: linux busybox initrd
	$(QEMU) -nographic -machine virt \
     -kernel linux/arch/riscv/boot/Image -append "root=/dev/vda ro console=ttyS0" \
     -drive file=busybox/busybox,format=raw,id=hd0 \
     -device virtio-blk-device,drive=hd0 \
     -initrd initramfs-busybox-riscv64.cpio.gz

clean:
	rm -Rf baremetal
	rm -Rf generic-elf
	rm -Rf initramfs

prereqs:
	# OSX brew install gawk gnu-sed gmp mpfr libmpc isl zlib expat
	# OSX brew install riscv-tools qemu
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
		gcc-riscv64-linux-gnu \
		git \
		libexpat-dev \
		libglib2.0-dev \
		libpixman-1-dev \
		libgmp-dev \
		libmpc-dev \
		libmpfr-dev \
		libtool \
		patchutils \
		pkg-config \
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

.PHONY: baremetal
baremetal:
	./scripts/build-baremetal.sh

.PHONY: elf
elf:
	./scripts/build-elf.sh
