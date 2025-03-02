#!/bin/bash
# set命令参考：https://www.ruanyifeng.com/blog/2017/11/bash-set.html
set -xue

# QEMU可执行程序
QEMU=qemu-system-riscv32

# clang命令及编译器标志
CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding  -nostdlib"

# 编译应用程序 shell
OBJCOPY=/usr/bin/llvm-objcopy
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
$OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o

# 编译内核
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf kernel.c common.c shell.bin.o

# 启动QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -drive id=drive0,file=lorem.txt,format=raw,if=none \
    -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
    -kernel kernel.elf

# # 启动QEMU并添加日志输出
# $QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
# -d unimp,guest_errors,int,cpu_reset -D qemu.log -kernel kernel.elf