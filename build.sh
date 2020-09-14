#!/usr/bin/env bash
export CXX=clang++-10
set -e

mkdir -p build_clang
pushd build_clang
cmake .. -DCMAKE_BUILD_TYPE=Release -DLTO=ON -DRISCV_ICACHE=ON -DRISCV_EXPERIMENTAL=ON
make -j8
./bench
popd
