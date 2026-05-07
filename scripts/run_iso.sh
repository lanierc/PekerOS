#!/bin/bash
qemu-system-i386 -m 256 -cdrom output/pekeros.iso -drive format=raw,file=output/disk.img -vga std -serial stdio
