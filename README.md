# PekerOS (FerkanOS)

PekerOS is a custom-built, 32-bit x86 operating system developed entirely from scratch. Starting from a Multiboot-compliant bootloader, it has evolved into a comprehensive system featuring virtual memory management, a graphical user interface, userland ELF application support, and a hardware-level networking stack.

---

## Key Features

- **Advanced Memory Management:** Implements 4KB Paging, a Physical Memory Manager (PMM), and a dynamic kernel heap (`kmalloc`/`kfree`).
- **Custom File System (PAFS):** Features the Peker Advanced File System (PAFS), working seamlessly through a Virtual File System (VFS) abstraction layer.
- **Userland & ELF Loader:** Capable of parsing and loading standard `ELF32` binaries from disk and executing them in Ring 3 (User Mode).
- **POSIX System Calls:** Provides a LibC-compatible syscall interface (`int 0x80`) supporting essential operations such as `SYS_OPEN`, `SYS_READ`, `SYS_WRITE_FD`, `SYS_CLOSE`, and `SYS_SBRK`.
- **Built-in Text Editor:** Includes a native, vi-like text editor (Mini-Vi) capable of reading, editing, and saving files directly to the disk.
- **Networking Stack:** Integrates a robust RTL8139 ethernet driver with a custom network protocol stack that parses Ethernet frames and responds to ARP Requests and ICMP Echo Requests (Ping).
- **Graphics Engine:** Supports VBE (VESA BIOS Extensions) for 800x600 32-bit graphical modes, featuring double-buffering and an interrupt-driven PS/2 mouse driver.

---

## Architecture & Milestones

The development of PekerOS is structured into focused phases:

### Phase 1-3: Core Foundations
- Implementation of the Global Descriptor Table (GDT), Interrupt Descriptor Table (IDT), and ISR/IRQ handling.
- Programmable Interrupt Controller (PIC) remapping.
- Hardware drivers for the Programmable Interval Timer (PIT) and PS/2 Keyboard.
- Development of the internal kernel shell, `fash` (Ferkan Advanced Shell).

### Phase 4: Graphical User Interface
- Transition from legacy VGA Text Mode to high-resolution VESA graphics.
- Implementation of a PCI Bus enumerator to discover hardware components.
- Development of an asynchronous, interrupt-driven PS/2 mouse driver with smooth cursor rendering.

### Phase 5: Userland & Applications
- Established the Ring 3 User Mode environment.
- Implemented the Syscall API for kernel-userland communication.
- Enabled dynamic memory allocation (`sbrk`) for userland applications.
- Ported the `vi` editor and developed user-space utilities.

### Phase 6: Networking
- Automatic PCI discovery and initialization of the RTL8139 Network Interface Card.
- DMA-based RX/TX buffer allocation for zero-copy packet handling.
- Implementation of ARP and IPv4 ICMP protocols, allowing the OS to reply to network discovery and Ping requests.

---

## Directory Structure

| Directory | Description |
|---|---|
| `boot/` | Assembly (NASM) code for the Multiboot-compliant bootloader |
| `kernel/` | Core C source code (Memory, Interrupts, Drivers, VFS, Network) |
| `include/` | Header files for kernel subsystems and drivers |
| `userland/` | Ring 3 user applications compiled as ELF binaries (e.g., `vi.c`, `hello.c`) |
| `scripts/` | Bash and Python utility scripts for building, disk formatting, and QEMU execution |
| `output/` | Compiled `.bin` kernel and the final bootable `disk.img` |

---

## Build and Run Instructions

Building and testing PekerOS requires a standard cross-compilation environment.

### Prerequisites
- GCC (i686-elf or standard gcc with `m32` support)
- NASM
- QEMU (`qemu-system-i386`)
- Python 3 (for PAFS disk injection tools)

### Building the OS

To compile the kernel and hardware drivers:
```bash
bash scripts/compile.sh
```

To compile userland applications and inject them into the disk image:
```bash
cd userland
bash compile_user.sh
```

### Running the OS

To launch the operating system in QEMU with network support enabled:
```bash
bash scripts/run.sh
```
*Once booted, you can type `exec vi` in the `fash` terminal to test the text editor, or monitor the kernel logs as the OS automatically responds to ARP requests on the virtual network.*

---

## Developer
**Muhammed Yasir PEKER**

*PekerOS: Built with a passion for low-level engineering and operating system design.*
