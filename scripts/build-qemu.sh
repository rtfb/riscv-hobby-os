#!/bin/sh

cd qemu
git checkout v5.1.0
./configure --target-list=riscv64-softmmu
make -j $(nproc)
