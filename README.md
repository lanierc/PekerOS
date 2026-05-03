# PekerOS

PekerOS is a custom-built, 32-bit x86 microkernel-based operating system developed from scratch. It features a modern graphical interface, a robust multitasking engine, and a high-performance graphics driver architecture.

## 🚀 Recent Milestones: The Graphics Evolution
In Phase 4, PekerOS successfully transitioned from legacy VGA Text Mode to a modern VESA VBE (VESA BIOS Extensions) graphical environment.

### Key Technical Features:
- **VBE Graphics Engine:** Supports 800x600 resolution with 32-bit color depth. Integrated Bochs Graphics Adapter (BGA) support via PCI discovery for hardware-accelerated framebuffer access.
- **High-Performance Sprite Engine:** Implements a "Sprite Saving" (Pixel Recovery) technique. Instead of redrawing the full screen, the system only backups and restores the 8x8 pixel region under the cursor, reducing CPU load by 99.9%.
- **Advanced PS/2 Mouse Driver:** Interrupt-driven (IRQ12) with 3-byte packet synchronization. Features custom sign-bit logic and boundary enforcement for smooth, jitter-free cursor movement.
- **Multitasking Rendering Pipeline:** Leveraging the kernel's scheduler, graphics rendering is handled by a dedicated asynchronous task (Graphics Task), ensuring that UI updates do not block the system shell.
- **Bitmap Font Engine:** Custom 8x16 bitmap font renderer for high-readability text output in graphical modes.

## 🛠️ Core Architecture
- **Bootloader:** Multiboot compliant (tested with GRUB and QEMU).
- **Memory Management:** Integrated Paging and Physical Memory Manager (PMM).
- **File System:** PAFS (Peker Advanced File System).
- **Interrupt Handling:** Global Descriptor Table (GDT), Interrupt Descriptor Table (IDT), and Master/Slave PIC remapping (IRQ 0-15).
- **Shell:** `fash` (Ferkan Advanced Shell) featuring system commands and multitasking control.

## 🔨 Build and Run

To compile the kernel and drivers:
```bash
bash scripts/compile.sh
```

To launch the OS in QEMU:
```bash
bash scripts/run.sh
```

## 👨‍💻 Developer
**Muhammed Yasir PEKER**

---
*PekerOS: A passion for low-level engineering and operating system design.* 🚀
