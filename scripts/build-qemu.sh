#!/bin/sh

cd qemu
git checkout v5.1.0
./configure --target-list=riscv64-softmmu --enable-kvm --prefix=$(readlink -m ../qemu-build/)
make -j $(nproc)
make install
./configure --target-list=riscv32-softmmu --enable-kvm --prefix=$(readlink -m ../qemu-build/)
make -j $(nproc)
make install
