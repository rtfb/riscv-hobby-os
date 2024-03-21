QEMU_LAUNCHER := ./scripts/qemu-launcher.py
ifeq ($(DBG),1)
	QEMU_LAUNCHER += --debug
endif

RISCV64_GCC ?= riscv64-linux-gnu-gcc
ifeq (, $(shell which $(RISCV64_GCC)))
	RISCV64_GCC = riscv64-unknown-elf-gcc
endif
RISCV64_OBJDUMP ?= riscv64-linux-gnu-objdump
ifeq (, $(shell which $(RISCV64_OBJDUMP)))
	RISCV64_OBJDUMP = riscv64-unknown-elf-objdump
endif
RISCV64_OBJCOPY ?= riscv64-linux-gnu-objcopy
ifeq (, $(shell which $(RISCV64_OBJCOPY)))
	RISCV64_OBJCOPY = riscv64-unknown-elf-objcopy
endif

FREEDOM_SDK_DIR := sifive-freedom-toolchain
ifneq ("$(wildcard $(HOME)/binutils-gdb/bin/riscv-gdb)","")
GDB := $(HOME)/binutils-gdb/bin/riscv-gdb
else
GDB := $(FREEDOM_SDK_DIR)/*/bin/riscv64-unknown-elf-gdb
endif

# If the links below get outdated, head to https://www.sifive.com/software and
# download GNU Embedded Toolchain.
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	SIFIVE_TOOLCHAIN_URL := https://static.dev.sifive.com/dev-tools/freedom-tools/v2020.12/riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-linux-ubuntu14.tar.gz
endif
ifeq ($(UNAME), Darwin)
	SIFIVE_TOOLCHAIN_URL := https://static.dev.sifive.com/dev-tools/freedom-tools/v2020.12/riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-apple-darwin.tar.gz
endif

OUT := out
BINS := \
	$(OUT)/os_sifive_u \
	$(OUT)/os_sifive_e \
	$(OUT)/os_sifive_e32 \
	$(OUT)/os_sifive_u32 \
	$(OUT)/os_hifive1_revb \
	$(OUT)/os_ox64 \
	$(OUT)/os_star64 \
	$(OUT)/os_d1 \
	$(OUT)/os_virt

# This target makes all the binaries depend on existence (but not timestamp) of
# $(OUT), which lets us avoid repetitive 'mkdir -p out'
$(BINS): | $(OUT)

all: $(BINS)

# boot.S has to go first in the list of dependencies so that the _start
# function is the very first thing in the .text section. Keeping it in a
# separate variable to prevent an occasional re-sorting of source files from
# moving it down the list. Let's keep context.S second in order to be compiled
# somewhere very low in the address space in order to fit in the trampoline
# page.
BOOT = src/boot.S \
	src/context.S
BASE_DEPS = $(BOOT) \
	src/bakedinfs.c \
	src/baremetal-poweroff.S \
	src/cpu.c \
	src/drivers/drivers.c \
	src/drivers/hd44780/hd44780.c \
	src/drivers/uart/uart.c \
	src/fdt.c \
	src/fs.c \
	src/gpio.c \
	src/kernel.c \
	src/kprintf.c \
	src/kprintf.S \
	src/mem.c \
	src/pagealloc.c \
	src/pipe.c \
	src/plic.c \
	src/pmp.c \
	src/proc.c \
	src/proc_test.c \
	src/riscv.c \
	src/runflags.c \
	src/sbi.c \
	src/spinlock.c \
	src/string.c \
	src/syscalls.c \
	src/timer.c \
	src/vm-stub.c \
	user/src/errno.c \
	user/src/shell.c \
	user/src/userland.c \
	user/src/user-printf.c \
	user/src/user-printf.S \
	user/src/ustr.c \
	user/src/usyscalls.S

OS_SIFIVE_U_DEPS = $(BASE_DEPS) \
	src/drivers/uart/uart-generic.c \
	src/machine/qemu/timer.c \
	src/vm.c
OS_SIFIVE_U32_DEPS = $(BASE_DEPS) \
	src/drivers/uart/uart-generic.c \
	src/machine/qemu/timer.c
OS_SIFIVE_E_DEPS = $(BASE_DEPS) \
	src/drivers/uart/uart-generic.c \
	src/machine/qemu/timer.c
OS_SIFIVE_E32_DEPS = $(BASE_DEPS) \
	src/drivers/uart/uart-generic.c \
	src/machine/qemu/timer.c
OS_VIRT_DEPS = $(BASE_DEPS) \
	src/drivers/uart/uart-ns16550a.c \
	src/machine/qemu/timer.c \
	src/vm.c
OS_OX64_DEPS = $(BASE_DEPS) \
	src/drivers/uart/uart-ox64.c \
	src/timer-sbi.c
OS_STAR64_DEPS = $(BASE_DEPS) \
	src/drivers/uart/uart-dw-apb.c \
	src/timer-sbi.c \
	src/vm.c
OS_D1_DEPS = $(BASE_DEPS) \
	src/drivers/uart/uart-dw-apb.c \
	src/machine/d1/timer.c

.PHONY: run-u
run-u: $(OUT)/os_sifive_u
	$(QEMU_LAUNCHER) --binary=$<

.PHONY: run-e
run-e: $(OUT)/os_sifive_e
	$(QEMU_LAUNCHER) --binary=$<

.PHONY: run-e32
run-e32: $(OUT)/os_sifive_e32
	$(QEMU_LAUNCHER) --binary=$<

.PHONY: run-u32
run-u32: $(OUT)/os_sifive_u32
	$(QEMU_LAUNCHER) --binary=$<

.PHONY: run-virt
run-virt: $(OUT)/os_virt
	$(QEMU_LAUNCHER) --binary=$<

# Note:
# * if this target complains that it can't find riscv64-unknown-elf-gdb,
#   run make download-sifive-toolchain
# * .debug-session file is not there permanently; it's being created by
#   qemu-launcher for the duration of the session and is being cleaned up on
#   exit. The file contains the name of the binary to debug
# * qemu-launcher also creates a .gbdinit file so that gdb sets the correct
#   arch and automatically attaches to the target
.PHONY: gdb
gdb:
	$(GDB) $(shell cat .debug-session)

GCC_FLAGS=-static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles \
          -ffreestanding \
          -fno-plt -fno-pic \
          --param=case-values-threshold=20 \
          -Wa,-Iinclude \
          -Tsrc/kernel.ld -Iinclude -Iuser/inc

$(OUT)/os_sifive_u: ${OS_SIFIVE_U_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-g -include include/machine/qemu_u.h \
		${OS_SIFIVE_U_DEPS} -o $@

$(OUT)/os_sifive_u32: ${OS_SIFIVE_U32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wa,--defsym,XLEN=32 \
		-g -include include/machine/qemu_u.h \
		${OS_SIFIVE_U32_DEPS} -o $@

$(OUT)/os_sifive_e: ${OS_SIFIVE_E_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -g \
		-Wl,--defsym,RAM_SIZE=0x4000 \
		-include include/machine/qemu_e.h \
		${OS_SIFIVE_E_DEPS} -o $@

$(OUT)/os_sifive_e32: ${OS_SIFIVE_E32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 \
		-Wa,--defsym,XLEN=32 \
		-g -Wl,--defsym,RAM_SIZE=0x4000 \
		-include include/machine/qemu_e.h \
		${OS_SIFIVE_E32_DEPS} -o $@

$(OUT)/os_virt: ${OS_VIRT_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wa,--defsym,QEMU_EXIT=0x100000 -g \
		-include include/machine/qemu_virt.h \
		${OS_VIRT_DEPS} -o $@

$(OUT)/os_hifive1_revb: ${OS_SIFIVE_E32_DEPS}
	$(RISCV64_GCC) -march=rv32imac -mabi=ilp32 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20010000 \
		-Wa,--defsym,XLEN=32 \
		-Wl,--defsym,RAM_SIZE=0x4000 \
		-include include/machine/hifive1-revb.h \
		${OS_SIFIVE_E32_DEPS} -o $@

$(OUT)/os_ox64: ${OS_OX64_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,RAM_START=0x50200000 -g \
		-include include/machine/ox64.h \
		${OS_OX64_DEPS} -o $@

$(OUT)/os_ox64.bin: $(OUT)/os_ox64
	$(RISCV64_OBJCOPY) -O binary $< $@

$(OUT)/os_ox64.s: $(OUT)/os_ox64
	$(RISCV64_OBJDUMP) --source --all-headers --demangle --line-numbers --wide -D $< > $@

$(OUT)/os_star64: ${OS_STAR64_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,RAM_START=0x80200000 -g \
		-include include/machine/star64.h \
		${OS_STAR64_DEPS} -o $@

$(OUT)/os_star64.bin: $(OUT)/os_star64
	$(RISCV64_OBJCOPY) -O binary $< $@

$(OUT)/os_star64.s: $(OUT)/os_star64
	$(RISCV64_OBJDUMP) --source --all-headers --demangle --line-numbers --wide -D $< > $@

$(OUT)/os_d1: ${OS_D1_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,RAM_START=0x40200000 -g \
		-include include/machine/d1.h \
		${OS_D1_DEPS} -o $@

$(OUT)/os_d1.bin: $(OUT)/os_d1
	$(RISCV64_OBJCOPY) -O binary $< $@

$(OUT)/os_d1.s: $(OUT)/os_d1
	$(RISCV64_OBJDUMP) --source --all-headers --demangle --line-numbers --wide -D $< > $@

$(OUT)/test-output-u64.txt: $(OUT)/os_sifive_u
	@$(QEMU_LAUNCHER) --bootargs dry-run --timeout=5s --binary=$< > $@
	@diff -u testdata/want-output-u64.txt $@
	@echo "OK"

$(OUT)/test-output-u32.txt: $(OUT)/os_sifive_u32
	@$(QEMU_LAUNCHER) --bootargs dry-run --timeout=5s --binary=$< > $@
	@diff -u testdata/want-output-u32.txt $@
	@echo "OK"

$(OUT)/test-output-virt.txt: $(OUT)/os_virt
	@$(QEMU_LAUNCHER) --bootargs dry-run --timeout=5s --binary=$< > $@
	@diff -u testdata/want-output-virt.txt $@
	@echo "OK"

$(OUT)/smoke-test-output-u32.txt: $(OUT)/os_sifive_u32
	@$(QEMU_LAUNCHER) --bootargs test-script=/home/smoke-test.sh --timeout=5s --binary=$< > $@
	@diff -u testdata/want-smoke-test-output-u32.txt $@
	@echo "OK"

$(OUT)/smoke-test-output-u64.txt: $(OUT)/os_sifive_u
	@$(QEMU_LAUNCHER) --bootargs test-script=/home/smoke-test.sh --timeout=5s --binary=$< > $@
	@diff -u testdata/want-smoke-test-output-u64.txt $@
	@echo "OK"

$(OUT)/smoke-test-output-virt.txt: $(OUT)/os_virt
	@$(QEMU_LAUNCHER) --bootargs test-script=/home/smoke-test.sh --timeout=5s --binary=$< > $@
	@diff -u testdata/want-smoke-test-output-virt.txt $@
	@echo "OK"

$(OUT)/daemon-test-output-virt.txt: $(OUT)/os_virt
	@$(QEMU_LAUNCHER) --bootargs test-script=/home/daemon-test.sh --timeout=5s --binary=$< > $@
	@diff -u testdata/want-daemon-test-output-virt.txt $@
	@echo "OK"

$(OUT)/daemon-test-output-u64.txt: $(OUT)/os_sifive_u
	@$(QEMU_LAUNCHER) --bootargs test-script=/home/daemon-test.sh --timeout=5s --binary=$< > $@
	@diff -u testdata/want-daemon-test-output-u64.txt $@
	@echo "OK"

$(OUT)/daemon-test-output-tiny-stack.txt: $(OUT)/os_virt
	@$(QEMU_LAUNCHER) --bootargs tiny-stack=/home/daemon-test.sh --timeout=5s --binary=$< > $@
	@diff -u testdata/want-daemon-test-output-tiny-stack.txt $@
	@echo "OK"

$(OUT)/ls-test-output-virt.txt: $(OUT)/os_virt
	@$(QEMU_LAUNCHER) --bootargs test-script=/home/ls-test.sh --timeout=5s --binary=$< > $@
	@diff -u testdata/want-ls-test-output-virt.txt $@
	@echo "OK"

$(OUT)/smoke-test-output-tiny-stack.txt: $(OUT)/os_virt
	@$(QEMU_LAUNCHER) --bootargs tiny-stack=/home/smoke-test.sh --timeout=5s --binary=$< > $@
	@diff -u testdata/want-smoke-test-output-tiny-stack.txt $@
	@echo "OK"

$(OUT):
	mkdir -p $(OUT)

$(OUT)/os_sifive_u.s: $(OUT)/os_sifive_u
	$(RISCV64_OBJDUMP) -D $< > $@

$(OUT)/os_sifive_u32.s: $(OUT)/os_sifive_u32
	$(RISCV64_OBJDUMP) -D $< > $@

$(OUT)/os_sifive_u.rodata: $(OUT)/os_sifive_u
	$(RISCV64_OBJDUMP) -s -j .rodata $< > $@

$(OUT)/os_sifive_e32.s: $(OUT)/os_sifive_e32
	$(RISCV64_OBJDUMP) -D $< > $@

$(OUT)/os_sifive_e32.rodata: $(OUT)/os_sifive_e32
	$(RISCV64_OBJDUMP) -s -j .rodata $< > $@

$(OUT)/os_hifive1_revb.hex: $(OUT)/os_hifive1_revb
	$(RISCV64_OBJCOPY) -O ihex $< $@

$(OUT)/os_hifive1_revb.s: $(OUT)/os_hifive1_revb
	$(RISCV64_OBJDUMP) --source --all-headers --demangle --line-numbers --wide -D $< > $@

$(OUT)/os_virt.s: $(OUT)/os_virt
	$(RISCV64_OBJDUMP) -D $< > $@

# This target assumes a Segger J-Link software is installed on the system. Get it at
# https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack
.PHONY: flash-hifive1-revb
flash-hifive1-revb: $(OUT)/os_hifive1_revb.hex $(OUT)/os_hifive1_revb.s
	echo "loadfile $<\nrnh\nexit" \
		| JLinkExe -device FE310 -if JTAG -speed 4000 -jtagconf -1,-1 -autoconnect 1

.PHONY: debug-board
debug-board: $(OUT)/os_hifive1_revb
	JLinkGDBServer -device RISC-V -port 1234 &
	$(GDB) $< -ex "set remotetimeout 240" -ex "target extended-remote localhost:1234"

.PHONY: clean
clean:
	rm -Rf $(OUT)

.PHONY: tags
tags:
	ctags --recurse include/ src/ user/

# On OSX:
# brew tap riscv-software-src/riscv
# brew install riscv-tools
prereqs:
	sudo apt --yes install \
		build-essential \
		gcc-riscv64-linux-gnu

$(FREEDOM_SDK_DIR):
	mkdir -p $@
	wget -qO- "$(SIFIVE_TOOLCHAIN_URL)" | tar xzv -C "$@"

QEMU_DOCKER_IMG_NAME := qemu-image

# builds the docker image
.PHONY: build-qemu-image
build-qemu-image:
	docker build -f scripts/Dockerfile -t ${QEMU_DOCKER_IMG_NAME} .
