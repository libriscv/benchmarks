#!/bin/bash
set -e
export CC="clang-19 --target=riscv32"
export CXX="clang++-19 --target=riscv32"

mkdir -p build_clang
pushd build_clang
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DGCC_TRIPLE=riscv32-unknown-elf
make -j4
popd
