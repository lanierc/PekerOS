#!/bin/bash
# Userland derleme betiği

# Klasörleri oluştur
mkdir -p bin

echo "[1/3] hello.c derleniyor..."
gcc -m32 -c hello.c -o hello.o -ffreestanding -fno-stack-protector -fno-pie -fno-pic -O2

echo "[2/3] Linkleniyor (ELF32)..."
ld -m elf_i386 -T user.ld hello.o -o bin/hello.elf
strip bin/hello.elf

echo "[3/3] Disk imajina enjekte ediliyor..."
if [ -f ../output/disk.img ]; then
    python3 ../scripts/pafs_tool.py ../output/disk.img bin/hello.elf hello
else
    echo "Hata: ../output/disk.img bulunamadı! Önce kernel'i derleyin."
fi

rm hello.o
echo "Bitti! Simdi FerkanOS'u baslatip 'exec hello' yazabilirsiniz."
