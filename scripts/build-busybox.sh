#!/bin/sh

cd busybox
CROSS_COMPILE=riscv64-linux-gnu- make defconfig
sed -i '/CONFIG_STATIC/c\CONFIG_STATIC=y' .config
CROSS_COMPILE=riscv64-linux-gnu- make -j $(nproc)
CROSS_COMPILE=riscv64-linux-gnu- make install
