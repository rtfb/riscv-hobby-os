#!/bin/bash

RISCV64_GCC="riscv64-linux-gnu-gcc"
if ! command -v $RISCV64_GCC &> /dev/null
then
    RISCV64_GCC="riscv64-unknown-elf-gcc"
fi

mkdir -pv generic-elf
$RISCV64_GCC -static -o generic-elf/hello hello.c hiasm.S
