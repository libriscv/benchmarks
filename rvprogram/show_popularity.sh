riscv32-unknown-elf-objdump -d --show-raw-insn build/rvbinary | awk '{ print $3 }' | sort -n | uniq -c | sort -h
