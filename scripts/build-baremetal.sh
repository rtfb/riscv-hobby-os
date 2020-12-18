#!/bin/bash

RISCV64_GCC="riscv64-linux-gnu-gcc"
if ! command -v $RISCV64_GCC &> /dev/null
then
    RISCV64_GCC="riscv64-unknown-elf-gcc"
fi

mkdir -pv baremetal
$RISCV64_GCC -march=rv64g -mabi=lp64 -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -Tbaremetal-fib.ld baremetal-fib.s -o baremetal/fib
