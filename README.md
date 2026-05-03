# PekerOS

PekerOS is a hobbyist Unix-like operating system developed for the x86 architecture. The project focuses on implementing fundamental operating system concepts from scratch, including memory management, multitasking, and privilege isolation.

## Technical Specifications

### Architecture
- Target: x86 (i386)
- Bootloader: Multiboot compliant (GRUB/QEMU)
- Kernel: Higher-half monolithic-style kernel mapped at 0xC0000000

### Core Features

#### Privilege Isolation (Ring 3)
- Implementation of User Mode (Ring 3) using GDT descriptors.
- Hardware-based context switching support via Task State Segment (TSS).
- Secure kernel stack switching during interrupts.

#### System Call Interface (int 0x80)
- Dispatcher mechanism for Ring 3 applications to request kernel services.
- Supported syscalls: sys_write, sys_uptime, sys_exit.

#### Memory Management
- Physical Memory Manager (PMM): Bitmap-based management of 4KB frames.
- Virtual Memory (Paging): 32-bit recursive paging with 4MB initial identity mapping.
- Kernel Heap: Dynamic memory allocation using a doubly-linked list header mechanism (kmalloc/kfree).

#### Multitasking
- Preemptive multitasking using the PIT (Programmable Interval Timer).
- Round Robin scheduling algorithm.
- Assembly-level context switching (register state preservation).

#### Storage and File System (PaFS)
- PaFS (PekerOS Advanced File System): A custom inode-based file system.
- Disk Driver: ATA/IDE support for PIO mode data transfer.
- Metadata Management: Superblock, Inode tables, and bit-based block bitmaps.

## Project Structure

- /boot: Assembly entry point and initial page tables.
- /kernel: Core kernel logic (IDT, GDT, Paging, Tasking, Syscalls).
- /include: Header files and architectural definitions.
- /scripts: Build and emulation scripts.

## Build and Emulation

### Prerequisites
- GCC (i386-elf-gcc)
- NASM
- GNU Binutils (ld)
- QEMU

### Compilation
To compile the kernel and generate the binary image:
```bash
bash scripts/compile.sh
```

### Execution
To run PekerOS in the QEMU emulator:
```bash
bash scripts/run.sh
```

## Development Status

PekerOS is currently in Phase 3 of its development roadmap. Current stable features include virtual memory, preemptive multitasking, and a functional system call bridge. Future phases involve PCI bus enumeration and VESA VBE graphics support.

## Author
Muhammed Yasir PEKER

## License
This project is licensed under the MIT License. See the LICENSE file for details.
