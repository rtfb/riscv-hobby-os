#!/bin/bash

RISCV64_GCC="riscv64-linux-gnu-gcc"
if ! command -v $RISCV64_GCC &> /dev/null
then
    RISCV64_GCC="riscv64-unknown-elf-gcc"
fi

mkdir -pv baremetal
$RISCV64_GCC -march=rv32g -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld baremetal-fib.s baremetal-print.s -Wa,--defsym,UART=0x10013000 -Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,XLEN=32 -o baremetal/fib_sifive_e32
$RISCV64_GCC -march=rv64g -mabi=lp64  -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld baremetal-fib.s baremetal-print.s -Wa,--defsym,UART=0x10013000 -Wl,--defsym,ROM_START=0x20400000                      -o baremetal/fib_sifive_e
$RISCV64_GCC -march=rv64g -mabi=lp64  -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld baremetal-fib.s baremetal-print.s                                                                                     -o baremetal/fib_sifive_u

$RISCV64_GCC -march=rv32g -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld baremetal-hello.s baremetal-print.s -Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000 -Wa,--defsym,XLEN=32 -o baremetal/hello_sifive_e32
$RISCV64_GCC -march=rv64g -mabi=lp64  -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld baremetal-hello.s baremetal-print.s -Wl,--defsym,ROM_START=0x20400000 -Wa,--defsym,UART=0x10013000                      -o baremetal/hello_sifive_e
$RISCV64_GCC -march=rv64g -mabi=lp64  -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal.ld baremetal-hello.s baremetal-print.s                                                                                     -o baremetal/hello_sifive_u
