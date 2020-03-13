#!/bin/bash
set -e
export CC=clang-11
export CXX=clang++-11

mkdir -p build_clang
pushd build_clang
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
make -j4
popd
