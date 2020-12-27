#!/bin/sh

cd riscv-isa-sim
mkdir -p build
cd build
../configure --prefix=$(readlink -m ./build/) --target=riscv64-unknown-linux-gnu
make
make install
