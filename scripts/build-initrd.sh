#!/bin/bash

RISCV64_GCC="riscv64-linux-gnu-gcc"
if ! command -v $RISCV64_GCC &> /dev/null
then
    RISCV64_GCC="riscv64-unknown-elf-gcc"
fi

mkdir -pv initramfs/riscv64-busybox
cd initramfs/riscv64-busybox
mkdir -pv {bin,sbin,etc,proc,sys,usr/{bin,sbin}}
$RISCV64_GCC -static -o usr/bin/hello ../../hello.c ../../hiasm.S
cp -av ../../busybox/_install/* .
cp ../../scripts/init .
chmod +x init
find . -print0 \
	| cpio --null -ov --format=newc \
	| gzip -9 > ../../initramfs-busybox-riscv64.cpio.gz
