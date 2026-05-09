# FerkanOS (PekerOS)

FerkanOS, x86 mimarisi üzerinde çalışan ve sıfırdan geliştirilen 32-bit bir işletim sistemi projesidir. Multiboot uyumlu bir önyükleyici (bootloader) ile başlayan süreç, bugün gelişmiş bellek yönetimi, grafik arayüzü, ağ yığını ve kullanıcı modu (User Mode) desteği olan bir çekirdeğe dönüşmüştür.

## Teknik Özellikler

### Çekirdek ve Bellek Yönetimi
- **Sanal Bellek:** 4KB sayfalama (Paging) ve 4-seviyeli hiyerarşi hazırlığı.
- **Bellek Yöneticileri:** Fiziksel Bellek Yöneticisi (PMM) ve dinamik çekirdek yığını (Kernel Heap - kmalloc/kfree).
- **Çoklu Görev (Multitasking):** Round-robin zamanlayıcı (Scheduler) ve görevler arası context switching.
- **Sistem Çağrıları:** int 0x80 üzerinden erişilen POSIX uyumlu sistem çağrısı arayüzü.

### Dosya Sistemi ve VFS
- **VFS (Sanal Dosya Sistemi):** Mount noktaları, sembolik bağlar (Symlink) ve dosya sistemi soyutlama katmanı.
- **PAFS:** Projeye özel tasarlanmış, basit ve hızlı Peker Gelişmiş Dosya Sistemi.
- **Ext2 Desteği:** İkinci disk (ATA Slave) üzerinden Ext2 dosya sistemlerini (Revision 1+) okuma desteği.
- **Aygıt Yönetimi:** ATA IDE sürücüsü (Master/Slave), PCI veri yolu keşfi ve yönetimi.

### Ağ Yığını (Networking)
- **Sürücü Desteği:** RTL8139 ve E1000 ağ kartları için PCI tabanlı sürücüler.
- **Protokoller:** Ethernet, ARP, IPv4, ICMP (Ping), UDP ve TCP (3-yönlü el sıkışma desteğiyle).
- **Socket API:** Kullanıcı modu uygulamaları için BSD tarzı soket arayüzü.

### Kullanıcı Arayüzü ve Grafik
- **Grafik Motoru:** VESA VBE desteği (800x600 32-bit), çift tamponlama (Double Buffering).
- **Giriş Birimleri:** Klavye (Türkçe Q) ve PS/2 Fare sürücüleri.
- **Kabuk (fash):** Dosya yönetimi, uygulama çalıştırma ve sistem izleme komutlarını içeren gelişmiş komut satırı arayüzü.

## Proje Yapısı

| Dizin | Açıklama |
|---|---|
| `boot/` | Multiboot uyumlu önyükleyici montaj kodları (NASM) |
| `kernel/` | Çekirdek kaynak kodları (Bellek, Sürücüler, VFS, Network, Süreç Yönetimi) |
| `include/` | Çekirdek alt sistemleri ve sürücüler için başlık dosyaları |
| `userland/` | ELF formatında derlenen kullanıcı modu uygulamaları (vi, hello, oyunlar) |
| `scripts/` | Derleme, disk imajı oluşturma ve QEMU çalıştırma araçları |
| `output/` | Derlenmiş ikili dosyalar ve önyüklenebilir disk imajı (`disk.img`) |

## Derleme ve Çalıştırma

### Gereksinimler
- GCC (i686-elf veya `m32` destekli standart gcc)
- NASM
- QEMU (`qemu-system-i386`)
- Python 3 (Disk enjeksiyon araçları için)

### Kurulum ve Başlatma
1. Çekirdek ve sürücülerin derlenmesi:
   ```bash
   make all
   ```
2. Kullanıcı uygulamalarının derlenmesi ve diske yazılması:
   ```bash
   cd userland && bash compile_user.sh
   ```
3. İşletim sisteminin QEMU üzerinde başlatılması:
   ```bash
   make run
   ```

## Geliştirici
**Muhammed Yasir PEKER**

*Bu proje, düşük seviyeli sistem programlama ve işletim sistemi mimarileri üzerine yapılan derinlemesine bir çalışmanın ürünüdür.*
