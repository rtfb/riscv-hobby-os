#!/bin/sh

cd riscv-pk
mkdir -p build
cd build
../configure --prefix=$(readlink -m ./build/) --host=riscv64-linux-gnu
make
make install
