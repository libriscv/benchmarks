#!/usr/bin/env bash
set -e

mkdir -p build_clang
pushd build_clang
cmake .. -DCMAKE_BUILD_TYPE=Release -DLTO=ON
make -j8
CFLAGS=-O2 ./bench
popd
