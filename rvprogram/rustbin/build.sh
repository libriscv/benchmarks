#!/usr/bin/env bash
cargo rustc --release --target riscv32imac-unknown-none-elf -- -C link-args="-e _start -static --undefined=fastexit --undefined=empty_function --undefined=test"
