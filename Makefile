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

OUT := out
BINS := $(OUT)/test_sifive_u \
	$(OUT)/user_sifive_u \
	$(OUT)/test_sifive_e \
	$(OUT)/user_sifive_e \
	$(OUT)/test_sifive_e32 \
	$(OUT)/user_sifive_e32 \
	$(OUT)/test_sifive_u32 \
	$(OUT)/user_sifive_u32 \
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

TEST_DEPS = src/baremetal-fib.s src/baremetal-print.s src/baremetal-poweroff.s
USER_DEPS = src/boot.s src/baremetal-print.s \
			src/baremetal-poweroff.s src/userland.c src/kernel.c src/syscalls.c \
			src/pmp.c src/riscv.c src/fdt.c src/string.c src/proc_test.c
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

GCC_FLAGS=-static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles \
          -ffreestanding \
          -Tsrc/baremetal.ld -Iinclude

$(OUT)/test_sifive_u: ${TEST_SIFIVE_U_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		${TEST_SIFIVE_U_DEPS} -o $@

$(OUT)/user_sifive_u: ${USER_SIFIVE_U_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		${USER_SIFIVE_U_DEPS} -o $@

$(OUT)/test_sifive_u32: ${TEST_SIFIVE_U32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wa,--defsym,XLEN=32 \
		${TEST_SIFIVE_U32_DEPS} -o $@

$(OUT)/user_sifive_u32: ${USER_SIFIVE_U32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wa,--defsym,XLEN=32 \
		${USER_SIFIVE_U32_DEPS} -o $@

$(OUT)/test_sifive_e: ${TEST_SIFIVE_E_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		${TEST_SIFIVE_E_DEPS} -o $@

$(OUT)/user_sifive_e: ${USER_SIFIVE_E_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		${USER_SIFIVE_E_DEPS} -o $@

$(OUT)/test_sifive_e32: ${TEST_SIFIVE_E32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,XLEN=32 \
		${TEST_SIFIVE_E32_DEPS} -o $@

$(OUT)/user_sifive_e32: ${USER_SIFIVE_E32_DEPS}
	$(RISCV64_GCC) -march=rv32g -mabi=ilp32 $(GCC_FLAGS) \
		-Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 \
		-Wa,--defsym,XLEN=32 \
		${USER_SIFIVE_E32_DEPS} -o $@

$(OUT)/test_virt: ${TEST_VIRT_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wa,--defsym,UART=0x10000000 -Wa,--defsym,QEMU_EXIT=0x100000 \
		${TEST_VIRT_DEPS} -o $@

$(OUT)/user_virt: ${USER_VIRT_DEPS}
	$(RISCV64_GCC) -march=rv64g -mabi=lp64 $(GCC_FLAGS) \
		-Wa,--defsym,UART=0x10000000 -Wa,--defsym,QEMU_EXIT=0x100000 \
		${USER_VIRT_DEPS} -o $@

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

run-spike: elf
	$(SPIKE) $(RISCV_PK) generic-elf/hello bbl loader

clean:
	rm -Rf $(OUT)

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
	git clone https://github.com/riscv/riscv-isa-sim
	git clone https://github.com/riscv/riscv-pk

.PHONY: qemu
qemu:
	./scripts/build-qemu.sh

$(OUT)/generic-elf/hello:
	@mkdir -p $(OUT)/generic-elf
	$(RISCV64_GCC) -static -o $@ src/generic-elf/hello.c src/generic-elf/hiasm.S
