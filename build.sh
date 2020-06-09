#!/usr/bin/env bash
export CXX=clang++-11
set -e

mkdir -p build_clang
pushd build_clang
cmake .. -DCMAKE_BUILD_TYPE=Release -DLTO=ON -DRISCV_EXT_C=OFF -DRISCV_ICACHE=ON
make -j8
./bench
popd
