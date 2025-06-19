#!/bin/bash
set -e

GCC_TRIPLE="riscv64-unknown-elf"
export CC=$GCC_TRIPLE-gcc
export CXX=$GCC_TRIPLE-g++

mkdir -p $GCC_TRIPLE
pushd $GCC_TRIPLE
cmake .. -DGCC_TRIPLE=$GCC_TRIPLE -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
make -j4
popd

cp $GCC_TRIPLE/rvbinary ../rv64-binary
