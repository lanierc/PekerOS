#!/bin/bash
# Çıktı klasörünü oluştur
mkdir -p output

# Eski dosyaları temizle
rm -f output/*.o output/ferkanos.bin

# Assembly
nasm -f elf32 boot/boot.s -o output/boot.o

# C Derleme - ÖNEMLI: -fno-pie -fno-pic bare-metal kernel için şart!
CFLAGS="-m32 -std=gnu99 -ffreestanding -O2 -Wall -fno-stack-protector -fno-pie -fno-pic -mno-sse -mno-sse2 -mno-mmx -Iinclude"

gcc $CFLAGS -c kernel/kernel.c -o output/kernel.o
gcc $CFLAGS -c kernel/gdt.c -o output/gdt.o
gcc $CFLAGS -c kernel/idt.c -o output/idt.o
gcc $CFLAGS -c kernel/irq.c -o output/irq.o
gcc $CFLAGS -c kernel/keyboard.c -o output/keyboard.o
gcc $CFLAGS -c kernel/shell.c -o output/shell.o
gcc $CFLAGS -c kernel/timer.c -o output/timer.o
gcc $CFLAGS -c kernel/pmm.c -o output/pmm.o
gcc $CFLAGS -c kernel/paging.c -o output/paging.o
gcc $CFLAGS -c kernel/kheap.c -o output/kheap.o
gcc $CFLAGS -c kernel/fafs.c -o output/fafs.o
gcc $CFLAGS -c kernel/ata.c -o output/ata.o
gcc $CFLAGS -c kernel/task.c -o output/task.o
gcc $CFLAGS -c kernel/tss.c -o output/tss.o
gcc $CFLAGS -c kernel/syscall.c -o output/syscall.o

# Linkleme
ld -m elf_i386 -T linker.ld -o output/ferkanos.bin output/boot.o output/kernel.o output/gdt.o output/idt.o output/irq.o output/keyboard.o output/shell.o output/timer.o output/pmm.o output/paging.o output/kheap.o output/fafs.o output/ata.o output/task.o output/tss.o output/syscall.o

# Sanal Disk Oluştur (10 MB, eğer yoksa)
if [ ! -f output/disk.img ]; then
    dd if=/dev/zero of=output/disk.img bs=1M count=10 status=none
fi