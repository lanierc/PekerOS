#!/bin/bash
# QEMU'yu başlat (Standart VGA/VBE)
qemu-system-i386 -m 256 -drive format=raw,file=output/disk.img -kernel output/pekeros.bin -vga std