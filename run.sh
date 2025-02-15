#!/bin/bash
# set命令参考：https://www.ruanyifeng.com/blog/2017/11/bash-set.html
set -xue

# QEMU可执行程序
QEMU=qemu-system-riscv32

# clang命令及编译器标志
CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding  -nostdlib"

# 编译内核
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf kernel.c common.c

# 启动QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot -kernel kernel.elf