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

FREEDOM_SDK_DIR := sifive-freedom-toolchain
GDB := $(FREEDOM_SDK_DIR)/*/bin/riscv64-unknown-elf-gdb

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
BINS := $(OUT)/test_sifive_u \
	$(OUT)/user_sifive_u \
	$(OUT)/test_sifive_e \
	$(OUT)/user_sifive_e \
	$(OUT)/test_sifive_e32 \
	$(OUT)/user_sifive_e32 \
	$(OUT)/test_sifive_u32 \
	$(OUT)/user_sifive_u32 \
	$(OUT)/user_hifive1_revb \
	$(OUT)/test_virt \
	$(OUT)/user_virt

# This target makes all the binaries depend on existence (but not timestamp) of
# $(OUT), which lets us avoid repetitive 'mkdir -p out'
$(BINS): | $(OUT)

all: $(BINS)
baremetal: all

# Shortcuts
run32: run-baremetal32
runm: run-baremetal
runb: run-baremetal
runs: run-spike

TEST_DEPS = src/baremetal-fib.s src/uart-print.s src/baremetal-poweroff.s
USER_DEPS = src/boot.s src/uart-print.s \
			src/baremetal-poweroff.s src/userland.c src/kernel.c src/syscalls.c \
			src/pmp.c src/riscv.c src/fdt.c src/string.c src/proc_test.c \
			src/spinlock.c src/proc.c src/usyscalls.S src/context.s \
			src/pagealloc.c src/uart.c src/user-printf.s src/user-printf.c \
			src/fs.c src/bakedinfs.c src/runflags.c
TEST_SIFIVE_U_DEPS = $(TEST_DEPS)
USER_SIFIVE_U_DEPS = $(USER_DEPS)
TEST_SIFIVE_E_DEPS = $(TEST_DEPS)
USER_SIFIVE_E_DEPS = $(USER_DEPS)
TEST_SIFIVE_E32_DEPS = $(TEST_DEPS)
USER_SIFIVE_E32_DEPS = $(USER_DEPS)
TEST_SIFIVE_U32_DEPS = $(TEST_DEPS)
USER_SIFIVE_U32_DEPS = $(USER_DEPS)
TEST_VIRT_DEPS = $(TEST_DEPS)
USER_VIRT_DEPS = $(USER_DEPS)

.PHONY: run-baremetal
run-baremetal: $(OUT)/user_sifive_u
	$(QEMU_LAUNCHER) --binary=$<

.PHONY: run-baremetale64
run-baremetale64: $(OUT)/user_sifive_e
	$(QEMU_LAUNCHER) --binary=$<

.PHONY: run-baremetal32
run-baremetal32: $(OUT)/user_sifive_e32
	$(QEMU_LAUNCHER) --binary=$<

.PHONY: run-baremetalu32
run-baremetalu32: $(OUT)/user_sifive_u32
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
gdb: $(FREEDOM_SDK_DIR)
	$(GDB) $(shell cat .debug-session)

GCC_FLAGS=-static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles \
          -ffreestanding \
          -fno-plt -fno-pic \
          -Tsrc/baremetal.ld -Iinclude

$(OUT)/user_sifive_u: ${USER_SIFIVE_U_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wa,--defsym,NUM_HARTS=2 -g \
		-include include/machine/qemu.h \
		${USER_SIFIVE_U_DEPS} -o $@

$(OUT)/user_sifive_u32: ${USER_SIFIVE_U32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wa,--defsym,XLEN=32 \
		-Wa,--defsym,NUM_HARTS=2 -g \
		-include include/machine/qemu.h \
		${USER_SIFIVE_U32_DEPS} -o $@

$(OUT)/user_sifive_e: ${USER_SIFIVE_E_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,NUM_HARTS=1 -g \
		-D UART_BASE=0x10013000 \
		-include include/machine/qemu.h \
		${USER_SIFIVE_E_DEPS} -o $@

$(OUT)/user_sifive_e32: ${USER_SIFIVE_E32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,XLEN=32 \
		-Wa,--defsym,NUM_HARTS=1 -g \
		-Wl,--defsym,RAM_SIZE=0x4000 \
		-D UART_BASE=0x10013000 \
		-include include/machine/qemu.h \
		${USER_SIFIVE_E32_DEPS} -o $@

$(OUT)/user_virt: ${USER_VIRT_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wa,--defsym,UART=0x10000000 -Wa,--defsym,QEMU_EXIT=0x100000 \
		-Wa,--defsym,NUM_HARTS=1 -g \
		-D UART_BASE=0x10000000 \
		-include include/machine/qemu.h \
		${USER_SIFIVE_E32_DEPS} -o $@

$(OUT)/user_hifive1_revb: ${USER_SIFIVE_E32_DEPS}
	$(RISCV64_GCC) -march=rv32imac -mabi=ilp32 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20010000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,XLEN=32 -Wa,--defsym,NO_S_MODE=1 -Wa,--defsym,NUM_HARTS=1 \
		-Wl,--defsym,RAM_SIZE=0x4000 \
		-D UART_BASE=0x10013000 \
		-include include/machine/hifive1-revb.h \
		${USER_SIFIVE_E32_DEPS} -o $@

$(OUT)/test_sifive_u: ${TEST_SIFIVE_U_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		${TEST_SIFIVE_U_DEPS} -o $@

$(OUT)/test_sifive_u32: ${TEST_SIFIVE_U32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wa,--defsym,XLEN=32 \
		-Wa,--defsym,NUM_HARTS=1 \
		-include include/machine/qemu.h \
		${TEST_SIFIVE_U32_DEPS} -o $@

$(OUT)/test_sifive_e: ${TEST_SIFIVE_E_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,NUM_HARTS=1 \
		-D UART_BASE=0x10013000 \
		-include include/machine/qemu.h \
		${TEST_SIFIVE_E_DEPS} -o $@

$(OUT)/test_sifive_e32: ${TEST_SIFIVE_E32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,XLEN=32 \
		-Wa,--defsym,NUM_HARTS=1 \
		-D UART_BASE=0x10013000 \
		-include include/machine/qemu.h \
		${TEST_SIFIVE_E32_DEPS} -o $@

$(OUT)/test_virt: ${TEST_VIRT_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wa,--defsym,UART=0x10000000 -Wa,--defsym,QEMU_EXIT=0x100000 \
		-D UART_BASE=0x10000000 \
		-include include/machine/qemu.h \
		${TEST_VIRT_DEPS} -o $@

.PHONY: test
test: $(OUT)/test-output.txt
	@diff -u testdata/want-output.txt $<
	@echo "OK"

$(OUT)/test-output.txt: $(OUT)/test_virt
	@$(QEMU_LAUNCHER) --machine=virt --binary=$< > $@

$(OUT)/test-output-u64.txt: $(OUT)/user_sifive_u
	@$(QEMU_LAUNCHER) --bootargs dry-run --timeout=5s --binary=$< > $@
	@diff -u testdata/want-output-u64.txt $@
	@echo "OK"

$(OUT)/test-output-u32.txt: $(OUT)/user_sifive_u32
	@$(QEMU_LAUNCHER) --bootargs dry-run --timeout=5s --binary=$< > $@
	@diff -u testdata/want-output-u32.txt $@
	@echo "OK"

$(OUT)/smoke-test-output-u32.txt: $(OUT)/user_sifive_u32
	@$(QEMU_LAUNCHER) --bootargs smoke-test --timeout=5s --binary=$< > $@
	@diff -u testdata/want-smoke-test-output-u32.txt $@
	@echo "OK"

$(OUT)/smoke-test-output-u64.txt: $(OUT)/user_sifive_u
	@$(QEMU_LAUNCHER) --bootargs smoke-test --timeout=5s --binary=$< > $@
	@diff -u testdata/want-smoke-test-output-u64.txt $@
	@echo "OK"

$(OUT):
	mkdir -p $(OUT)

$(OUT)/user_sifive_u.s: $(OUT)/user_sifive_u
	$(RISCV64_OBJDUMP) -D $< > $@

$(OUT)/user_sifive_u.rodata: $(OUT)/user_sifive_u
	$(RISCV64_OBJDUMP) -s -j .rodata $< > $@

$(OUT)/user_sifive_e32.s: $(OUT)/user_sifive_e32
	$(RISCV64_OBJDUMP) -D $< > $@

$(OUT)/user_sifive_e32.rodata: $(OUT)/user_sifive_e32
	$(RISCV64_OBJDUMP) -s -j .rodata $< > $@

$(OUT)/user_hifive1_revb.hex: $(OUT)/user_hifive1_revb
	$(RISCV64_OBJCOPY) -O ihex $< $@

$(OUT)/user_hifive1_revb.s: $(OUT)/user_hifive1_revb
	$(RISCV64_OBJDUMP) --source --all-headers --demangle --line-numbers --wide -D $< > $@

# This target assumes a Segger J-Link software is installed on the system. Get it at
# https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack
.PHONY: flash-hifive1-revb
flash-hifive1-revb: $(OUT)/user_hifive1_revb.hex $(OUT)/user_hifive1_revb.s
	echo "loadfile $<\nrnh\nexit" \
		| JLinkExe -device FE310 -if JTAG -speed 4000 -jtagconf -1,-1 -autoconnect 1

.PHONY: debug-board
debug-board: $(OUT)/user_hifive1_revb
	JLinkGDBServer -device RISC-V -port 1234 &
	$(GDB) $< -ex "set remotetimeout 240" -ex "target extended-remote localhost:1234"

run-spike: elf
	$(SPIKE) $(RISCV_PK) generic-elf/hello bbl loader

clean:
	rm -Rf $(OUT)

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

clone:
	git clone https://github.com/riscv/riscv-isa-sim
	git clone https://github.com/riscv/riscv-pk

.PHONY: qemu
qemu:
	./scripts/build-qemu.sh

QEMU_DOCKER_IMG_NAME := qemu-image

# builds the docker image
.PHONY: build-qemu-image
build-qemu-image:
	docker build -f scripts/Dockerfile -t ${QEMU_DOCKER_IMG_NAME} .

$(OUT)/generic-elf/hello:
	@mkdir -p $(OUT)/generic-elf
	$(RISCV64_GCC) -static -o $@ src/generic-elf/hello.c src/generic-elf/hiasm.S
