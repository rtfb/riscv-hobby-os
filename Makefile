# If local QEmu build does not exist, use system wide installed QEmu
QEMU ?= ./qemu-build/bin/qemu-system-riscv64
ifeq ($(wildcard $(QEMU)),)
	QEMU = qemu-system-riscv64
endif
QEMU32 ?= ./qemu-build/bin/qemu-system-riscv32
ifeq ($(wildcard $(QEMU32)),)
	QEMU32 = qemu-system-riscv32
endif

RISCV64_GCC ?= riscv64-linux-gnu-gcc
ifeq (, $(shell which $(RISCV64_GCC)))
	RISCV64_GCC = riscv64-unknown-elf-gcc
endif
# Spike, the RISC-V ISA Simulator (https://github.com/riscv/riscv-isa-sim)
SPIKE ?= ./riscv-isa-sim/build/build/bin/spike
ifeq ($(wildcard $(SPIKE)),)
	SPIKE = spike
endif
# Proxy Kernel, a lightweight app execution environment that can host RISC-V ELF binaries (https://github.com/riscv/riscv-pk)
RISCV_PK ?= riscv-pk/build/build/riscv64-linux-gnu/bin/pk
ifeq ($(wildcard $(RISCV_PK)),)
	RISCV_PK = pk
endif

all: baremetal/hello_sifive_u \
	baremetal/fib_sifive_u \
	baremetal/user_sifive_u \
	baremetal/hello_sifive_e \
	baremetal/fib_sifive_e \
	baremetal/user_sifive_e \
	baremetal/hello_sifive_e32 \
	baremetal/fib_sifive_e32 \
	baremetal/user_sifive_e32 \
	baremetal/hello_virt \
	baremetal/fib_virt \
	baremetal/user_virt
baremetal: all

# Shortcuts
run32: run-baremetal32
runm: run-baremetal
runb: run-baremetal
runs: run-spike
runl: run-linux

FIB_DEPS = baremetal-fib.s baremetal-print.s baremetal-poweroff.s
HELLO_DEPS = baremetal-hello.s baremetal-print.s
USER_DEP = baremetal-user.s baremetal-print.s baremetal-poweroff.s
FIB_SIFIVE_U_DEPS = $(FIB_DEPS)
HELLO_SIFIVE_U_DEPS = $(HELLO_DEPS)
USER_SIFIVE_U_DEPS = $(USER_DEP)
FIB_SIFIVE_E_DEPS = $(FIB_DEPS)
HELLO_SIFIVE_E_DEPS = $(HELLO_DEPS)
USER_SIFIVE_E_DEPS = $(USER_DEP)
FIB_SIFIVE_E32_DEPS = $(FIB_DEPS)
HELLO_SIFIVE_E32_DEPS = $(HELLO_DEPS)
USER_SIFIVE_E32_DEPS = $(USER_DEP)
FIB_VIRT_DEPS = $(FIB_DEPS)
HELLO_VIRT_DEPS = $(HELLO_DEPS)
USER_VIRT_DEPS = $(USER_DEP)

.PHONY: run-baremetal
run-baremetal: baremetal/hello_sifive_u
	$(QEMU) -nographic -machine sifive_u -bios none -kernel $<

.PHONY: run-baremetal-fib
run-baremetal-fib: baremetal/fib_sifive_u
	$(QEMU) -nographic -machine sifive_u -bios none -kernel $<

.PHONY: run-baremetal-user
run-baremetal-user: baremetal/user_sifive_u
	$(QEMU) -nographic -machine sifive_u -bios none -kernel $<

.PHONY: run-baremetal32
run-baremetal32: baremetal/hello_sifive_e32
	$(QEMU32) -nographic -machine sifive_e -bios none -kernel $<

.PHONY: run-baremetal32-fib
run-baremetal32-fib: baremetal/fib_sifive_e32
	$(QEMU32) -nographic -machine sifive_e -bios none -kernel $<

.PHONY: run-baremetal32-user
run-baremetal32-user: baremetal/user_sifive_e32
	$(QEMU32) -nographic -machine sifive_e -bios none -kernel $<

baremetal/hello_sifive_u: ${HELLO_SIFIVE_U_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv64g -mabi=lp64  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		${HELLO_SIFIVE_U_DEPS} -o $@

baremetal/fib_sifive_u: ${FIB_SIFIVE_U_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv64g -mabi=lp64  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		${FIB_SIFIVE_U_DEPS} -o $@

baremetal/user_sifive_u: ${USER_SIFIVE_U_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv64g -mabi=lp64  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		${USER_SIFIVE_U_DEPS} -o $@

baremetal/hello_sifive_e: ${HELLO_SIFIVE_E_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv64g -mabi=lp64  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		${HELLO_SIFIVE_E_DEPS} -o $@

baremetal/fib_sifive_e: ${FIB_SIFIVE_E_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv64g -mabi=lp64  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		${FIB_SIFIVE_E_DEPS} -o $@

baremetal/user_sifive_e: ${USER_SIFIVE_E_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv64g -mabi=lp64  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		${USER_SIFIVE_E_DEPS} -o $@

baremetal/hello_sifive_e32: ${HELLO_SIFIVE_E32_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,XLEN=32 \
		${HELLO_SIFIVE_E32_DEPS} -o $@

baremetal/fib_sifive_e32: ${FIB_SIFIVE_E32_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,XLEN=32 \
		${FIB_SIFIVE_E32_DEPS} -o $@

baremetal/user_sifive_e32: ${USER_SIFIVE_E32_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,XLEN=32 \
		${USER_SIFIVE_E32_DEPS} -o $@

baremetal/hello_virt: ${HELLO_VIRT_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv64g -mabi=lp64  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		-Wa,--defsym,UART=0x10000000 -Wa,--defsym,QEMU_EXIT=0x100000 \
		${HELLO_VIRT_DEPS} -o $@

baremetal/fib_virt: ${FIB_VIRT_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv64g -mabi=lp64  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		-Wa,--defsym,UART=0x10000000 -Wa,--defsym,QEMU_EXIT=0x100000 \
		${FIB_VIRT_DEPS} -o $@

baremetal/user_virt: ${USER_VIRT_DEPS}
	@mkdir -p baremetal
	$(RISCV64_GCC) -march=rv64g -mabi=lp64  -static -mcmodel=medany \
		-fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld \
		-Wa,--defsym,UART=0x10000000 -Wa,--defsym,QEMU_EXIT=0x100000 \
		${USER_VIRT_DEPS} -o $@


run-spike: elf
	$(SPIKE) $(RISCV_PK) generic-elf/hello bbl loader

run-linux: initrd
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
		device-tree-compiler \
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
	git clone https://github.com/riscv/riscv-isa-sim
	git clone https://github.com/riscv/riscv-pk

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

.PHONY: elf
elf:
	./scripts/build-elf.sh
