#!/bin/bash
# Userland derleme betiği

# Klasörleri oluştur
mkdir -p bin

echo "[1/3] C kodlari derleniyor..."
gcc -m32 -c syscalls.c -o syscalls.o -ffreestanding -fno-stack-protector -fno-pie -fno-pic -Os
gcc -m32 -c hello.c -o hello.o -ffreestanding -fno-stack-protector -fno-pie -fno-pic -Os
gcc -m32 -c guess.c -o guess.o -ffreestanding -fno-stack-protector -fno-pie -fno-pic -Os
gcc -m32 -c vi.c -o vi.o -ffreestanding -fno-stack-protector -fno-pie -fno-pic -Os

echo "[2/3] Linkleniyor (ELF32)..."
ld -m elf_i386 -T user.ld syscalls.o hello.o -o bin/hello.elf
strip bin/hello.elf

ld -m elf_i386 -T user.ld syscalls.o guess.o -o bin/guess.elf
strip bin/guess.elf

ld -m elf_i386 -T user.ld syscalls.o vi.o -o bin/vi.elf
strip bin/vi.elf

echo "[3/3] Disk imajina enjekte ediliyor..."
if [ -f ../output/disk.img ]; then
    python3 ../scripts/pafs_tool.py ../output/disk.img bin/hello.elf hello
    python3 ../scripts/pafs_tool.py ../output/disk.img bin/guess.elf guess
    python3 ../scripts/pafs_tool.py ../output/disk.img bin/vi.elf vi
else
    echo "Hata: ../output/disk.img bulunamadı! Önce kernel'i derleyin."
fi

rm hello.o guess.o syscalls.o vi.o
echo "Bitti! Simdi FerkanOS'u baslatip 'exec vi' yazabilirsiniz."
