#!/bin/bash

qemu="/usr/bin/qemu-system-riscv64"

if [ "$1" == "rv32" ]; then
    qemu="/usr/bin/qemu-system-riscv32"
    shift
fi

$qemu $*
