#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#define MULTIBOOT_MAGIC 0x2BADB002

// Multiboot info flags
#define MB_FLAG_MEM     (1 << 0)   // mem_lower / mem_upper mevcut
#define MB_FLAG_MMAP    (1 << 6)   // mmap_addr / mmap_length mevcut

struct multiboot_info {
    unsigned int flags;
    unsigned int mem_lower;       // KB cinsinden (BIOS int 0x15, eax=0x88)
    unsigned int mem_upper;       // KB cinsinden (1MB üstü)
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int syms[4];
    unsigned int mmap_length;
    unsigned int mmap_addr;
} __attribute__((packed));

struct multiboot_mmap_entry {
    unsigned int size;            // Bu alanın kendisi hariç girdinin boyutu
    unsigned int base_addr_low;
    unsigned int base_addr_high;
    unsigned int length_low;
    unsigned int length_high;
    unsigned int type;            // 1 = Kullanılabilir RAM
} __attribute__((packed));

#define MMAP_TYPE_AVAILABLE 1

#endif
