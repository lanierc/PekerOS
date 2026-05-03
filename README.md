# PekerOS 🛡️

PekerOS, x86 mimarisi üzerinde sıfırdan geliştirilen, modern grafik arayüzüne ve gelişmiş sürücü desteğine sahip bir mikroçekirdek (microkernel) denemesidir.

## 🚀 Son Güncellemeler: Grafik Devrimi
PekerOS, Faz 4 kapsamında metin tabanlı arayüzden (VGA Text Mode) modern VESA VBE grafik moduna geçiş yapmıştır.

### Öne Çıkan Özellikler:
- **Grafik Motoru (VBE):** 800x600 çözünürlükte 32-bit renk derinliği. Bochs Graphics Adapter (BGA) üzerinden PCI tabanlı donanım hızlandırma desteği.
- **Ultra-Hızlı İmleç Sistemi:** "Sprite Saving" tekniği ile sadece 8x8 piksellik alanlar güncellenerek %99.9 performans artışı sağlandı.
- **PS/2 Fare Sürücüsü:** Kesme tabanlı (IRQ12), 3-byte paket senkronizasyonlu ve matematiksel işaret düzeltmeli pürüzsüz imleç hareketi.
- **Multitasking Render:** Grafik çizim işlemleri, çekirdeğin multitasking yapısı kullanılarak bağımsız bir görev (Graphics Task) üzerinden asenkron olarak yürütülür.
- **Bitmap Font Sistemi:** 8x16 bitmap font motoru ile grafik modunda yüksek okunabilirlikli metin çıktısı.

## 🛠️ Teknik Altyapı
- **Bootloader:** Multiboot uyumlu (GRUB/QEMU).
- **Bellek Yönetimi:** Paging (Sayfalama) ve Fiziksel Bellek Yönetimi (PMM).
- **Dosya Sistemi:** PAFS (Peker Advanced File System).
- **Kesme Yönetimi:** GDT, IDT ve PIC Remapping (IRQ 0-15).
- **Kabuk:** `fash` (Ferkan Advanced Shell).

## 🔨 Derleme ve Çalıştırma

Sistemi derlemek için:
```bash
bash scripts/compile.sh
```

QEMU üzerinde çalıştırmak için:
```bash
bash scripts/run.sh
```

## 👨‍💻 Geliştirici
**Muhammed Yasir PEKER**

---
*PekerOS, bir işletim sisteminden daha fazlası; bir mühendislik tutkusudur.* 🚀
