# QEMU'yu başlat
qemu-system-i386 -kernel output/pekeros.bin -drive file=output/disk.img,format=raw,index=0,media=disk -serial stdio