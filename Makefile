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
BINS := \
	$(OUT)/user_sifive_u \
	$(OUT)/user_sifive_e \
	$(OUT)/user_sifive_e32 \
	$(OUT)/user_sifive_u32 \
	$(OUT)/user_hifive1_revb \
	$(OUT)/user_ox64_u \
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

USER_DEPS = src/boot.S src/kprintf.c src/kprintf.S src/baremetal-poweroff.S src/kernel.c \
			src/syscalls.c src/pmp.c src/riscv.c src/fdt.c src/string.c \
			src/proc_test.c src/spinlock.c src/proc.c src/context.S \
			src/pagealloc.c src/fs.c src/bakedinfs.c src/runflags.c \
			src/pipe.c src/drivers/drivers.c src/drivers/uart/uart.c \
			src/drivers/hd44780/hd44780.c src/plic.c src/gpio.c \
			user/src/userland.c user/src/usyscalls.S user/src/user-printf.S \
			user/src/user-printf.c user/src/shell.c user/src/ustr.c
TEST_SIFIVE_U_DEPS = $(TEST_DEPS)
USER_SIFIVE_U_DEPS = $(USER_DEPS) src/timer.c src/drivers/uart/uart-generic.c
TEST_SIFIVE_E_DEPS = $(TEST_DEPS)
USER_SIFIVE_E_DEPS = $(USER_DEPS) src/timer.c src/drivers/uart/uart-generic.c
TEST_SIFIVE_E32_DEPS = $(TEST_DEPS)
USER_SIFIVE_E32_DEPS = $(USER_DEPS) src/timer.c src/drivers/uart/uart-generic.c
TEST_SIFIVE_U32_DEPS = $(TEST_DEPS)
USER_SIFIVE_U32_DEPS = $(USER_DEPS) src/timer.c src/drivers/uart/uart-generic.c
TEST_VIRT_DEPS = $(TEST_DEPS)
USER_VIRT_DEPS = $(USER_DEPS) src/timer.c src/drivers/uart/uart-generic.c
USER_OX64_U_DEPS = $(USER_DEPS) \
			src/machine/ox64/timer.c src/drivers/uart/uart-ox64.c

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
          --param=case-values-threshold=20 \
          -Wa,-Iinclude \
          -Tsrc/kernel.ld -Iinclude -Iuser/inc

$(OUT)/user_sifive_u: ${USER_SIFIVE_U_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-g -include include/machine/qemu_u.h \
		${USER_SIFIVE_U_DEPS} -o $@

$(OUT)/user_sifive_u32: ${USER_SIFIVE_U32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wa,--defsym,XLEN=32 \
		-g -include include/machine/qemu_u.h \
		${USER_SIFIVE_U32_DEPS} -o $@

$(OUT)/user_sifive_e: ${USER_SIFIVE_E_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -g \
		-include include/machine/qemu_e.h \
		${USER_SIFIVE_E_DEPS} -o $@

$(OUT)/user_sifive_e32: ${USER_SIFIVE_E32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 \
		-Wa,--defsym,XLEN=32 \
		-g -Wl,--defsym,RAM_SIZE=0x4000 \
		-include include/machine/qemu_e.h \
		${USER_SIFIVE_E32_DEPS} -o $@

$(OUT)/user_virt: ${USER_VIRT_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wa,--defsym,QEMU_EXIT=0x100000 -g \
		-include include/machine/qemu_virt.h \
		${USER_SIFIVE_E32_DEPS} -o $@

$(OUT)/user_hifive1_revb: ${USER_SIFIVE_E32_DEPS}
	$(RISCV64_GCC) -march=rv32imac -mabi=ilp32 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20010000 \
		-Wa,--defsym,XLEN=32 \
		-Wl,--defsym,RAM_SIZE=0x4000 \
		-include include/machine/hifive1-revb.h \
		${USER_SIFIVE_E32_DEPS} -o $@

$(OUT)/user_ox64_u: ${USER_OX64_U_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,RAM_START=0x50000000 -g \
		-include include/machine/ox64.h \
		${USER_OX64_U_DEPS} -o $@

$(OUT)/user_ox64_u.bin: $(OUT)/user_ox64_u
	$(RISCV64_OBJCOPY) -O binary $< $@

$(OUT)/user_ox64_u.s: $(OUT)/user_ox64_u
	$(RISCV64_OBJDUMP) --source --all-headers --demangle --line-numbers --wide -D $< > $@

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

.PHONY: qemu
qemu:
	./scripts/build-qemu.sh

QEMU_DOCKER_IMG_NAME := qemu-image

# builds the docker image
.PHONY: build-qemu-image
build-qemu-image:
	docker build -f scripts/Dockerfile -t ${QEMU_DOCKER_IMG_NAME} .
