#!/bin/bash

mkdir -pv initramfs/riscv64-busybox
cd initramfs/riscv64-busybox
mkdir -pv {bin,sbin,etc,proc,sys,usr/{bin,sbin}}
riscv64-linux-gnu-gcc -static -o usr/bin/hello ../../hello.c ../../hiasm.S
cp -av ../../busybox/_install/* .
cp ../../scripts/init .
chmod +x init
find . -print0 \
	| cpio --null -ov --format=newc \
	| gzip -9 > ../../initramfs-busybox-riscv64.cpio.gz
