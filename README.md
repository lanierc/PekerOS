# 🚀 PekerOS (FerkanOS)

PekerOS, x86 (32-bit) mimarisi için sıfırdan (from scratch) geliştirilmiş, tamamen bağımsız ve modern özelliklerle donatılmış hobi amaçlı bir işletim sistemidir. Multiboot uyumlu bir bootloader ile başlayan bu yolculuk; sanal bellek yönetiminden (Paging), grafik kullanıcı arayüzüne (VBE), kullanıcı alanı (Userland) ELF uygulamalarından, donanımsal ağ sürücülerine (RTL8139) kadar uzanan devasa bir ekosisteme dönüşmüştür.

---

## 🌟 Öne Çıkan Özellikler

- **Gelişmiş Bellek Yönetimi**: 4KB sayfalama (Paging), PMM (Physical Memory Manager) ve kheap (Kernel Heap) ile dinamik bellek yönetimi (kmalloc/kfree).
- **Kendi Dosya Sistemimiz (PAFS)**: İşletim sistemine özel olarak tasarlanmış *Peker Advanced File System (PAFS)*. VFS (Virtual File System) katmanı üzerinden çalışır.
- **Kullanıcı Alanı (Userland) ve ELF Yükleyici**: Diskten `ELF32` formatındaki binary dosyaları belleğe alıp ayrıştırarak Ring 3 (User Mode) seviyesinde çalıştırabilen yerleşik bir ELF Loader.
- **POSIX Sistem Çağrıları (LibC Uyumluluğu)**: Uygulamalar standart C fonksiyonlarını kullanabilsin diye tasarlanmış sistem çağrıları (Syscalls). Desteklenen çağrılar: `SYS_OPEN`, `SYS_READ`, `SYS_WRITE_FD`, `SYS_CLOSE`, `SYS_SBRK`, `SYS_CLEAR`.
- **Dahili Metin Editörü (Mini-Vi)**: Diskten bir dosya açıp, içerisinde değişiklik yapıp kalıcı olarak kaydedebileceğiniz Nano/Vi benzeri yerleşik metin editörü (Pilo).
- **Ağ Yığını (Network Stack)**: Gerçek bir PCI donanımı olan **RTL8139** ethernet kartı sürücüsü. Ethernet Frame, ARP Request/Reply ve IPv4 ICMP Ping Echo Reply protokollerini tamamen anlayan ve cevap verebilen bir ağ katmanı.
- **Grafik ve Multimedya**: VBE (VESA BIOS Extensions) üzerinden 800x600 32-bit çözünürlük desteği. Çift tamponlama (Double Buffering) ve donanım kesmeli PS/2 fare desteği.

---

## 🛠️ Mimari ve Gelişim Aşamaları (Fazlar)

PekerOS, yapılandırılmış fazlar halinde geliştirilmiştir:

### 🟢 Faz 1-3: Çekirdek (Kernel) Temelleri
- GDT, IDT ve IRQ/ISR (Donanım kesmeleri) kurulumu.
- PIC (Programmable Interrupt Controller) yeniden haritalandırması.
- PIT (Zamanlayıcı) ve Klavye donanım sürücüleri.
- `fash` (Ferkan Advanced Shell) isminde dahili bir komut satırı arayüzü.

### 🔵 Faz 4: Grafik Dünyası (GUI)
- Legacy VGA Metin Modundan, VESA Grafik Moduna geçiş.
- PCI Veri Yolu (Bus) tarayıcısı ile donanımların keşfedilmesi.
- PS/2 Fare (Mouse) Sürücüsü entegrasyonu (IRQ12) ve yumuşak imleç (Cursor) oluşturulması.

### 🟣 Faz 5: Userland ve Uygulamalar
- Kullanıcıların kendi C kodlarını derleyip işletim sistemine yükleyebilmesi sağlandı.
- **Syscall API:** `int 0x80` üzerinden çekirdek-kullanıcı haberleşmesi.
- `sbrk` kullanılarak uygulamaların kendi içinde `malloc()` yapabilmesi için dinamik bellek büyüme yeteneği.
- `vi` editörü ve çeşitli test oyunları (Guess vb.) geliştirildi.

### 🟠 Faz 6: Ağ Dünyası (Networking)
- **PCI üzerinden RTL8139 Keşfi:** Sistem boot anında ağ kartını bulup donanımı aktifleştirir.
- **DMA Tabanlı RX/TX Buffer:** Ağ paketleri işlemciyi yormadan doğrudan DMA üzerinden tahsis edilen fiziksel çerçevelere (frames) aktarılır.
- **ARP & ICMP:** Ağdaki bilgisayarlardan gelen "Benimle konuşur musun?" (ARP) ve "Orada mısın?" (Ping) isteklerini algılayıp, MAC ve IP adreslerini kendi kendine doldurarak **Echo Reply** gönderen muazzam bir alt sistem.

---

## 📂 Proje Dizin Yapısı

| Dizin | Açıklama |
|---|---|
| `boot/` | Assembly (NASM) ile yazılmış Multiboot uyumlu başlangıç kodları |
| `kernel/` | İşletim sisteminin ana C kodları (GDT, Kesmeler, Paging, Aygıt Sürücüleri, VFS, Network) |
| `include/` | Kernel ve modüller için yazılmış tüm `.h` (header) dosyaları |
| `userland/` | İşletim sistemi üzerinde çalışan, diskten ELF olarak yüklenen kullanıcı (Ring 3) uygulamaları (Örn: `vi.c`, `hello.c`) |
| `scripts/` | QEMU'yu başlatmak, diski formatlamak, PAFS'a uygulama enjekte etmek için kullanılan yardımcı bash ve python araçları |
| `output/` | Derleme sonrası oluşan `.bin` ve `disk.img` (İşletim sistemi kalıbı) |

---

## 🚀 Kurulum ve Çalıştırma

PekerOS'u kendi bilgisayarınızda derleyip test etmek oldukça kolaydır. 

### Gereksinimler
- GCC (i686-elf veya standart gcc `m32` desteğiyle)
- NASM
- QEMU (`qemu-system-i386`)
- Python 3 (Disk enjeksiyon araçları için)

### Derleme (Build)

Kernel'i ve sürücüleri derlemek için:
```bash
bash scripts/compile.sh
```

Kullanıcı uygulamalarını (Vi editörü vb.) derleyip diske enjekte etmek için:
```bash
cd userland
bash compile_user.sh
```

### Başlatma (Run)

İşletim sistemini QEMU sanal makinesi üzerinde, RTL8139 Ağ Kartı desteğiyle çalıştırmak için:
```bash
bash scripts/run.sh
```
*Sistem açıldığında, `fash` terminaline `exec vi` yazarak dahili editörü test edebilir veya host makinenizin ağ izleyicileriyle işletim sisteminizin ARP yanıtlarını gözlemleyebilirsiniz.*

---

## 👨‍💻 Geliştirici
**Muhammed Yasir PEKER**

*PekerOS: Düşük seviye mühendisliğe ve işletim sistemi tasarımına duyulan tutkunun bir eseridir.* 💻✨
