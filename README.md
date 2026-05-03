# 🌌 PekerOS

PekerOS, x86 mimarisi üzerinde sıfırdan geliştirilen, UNIX benzeri (UNIX-like) bir hobi işletim sistemidir. Bu proje, çekirdek (kernel) seviyesinde bellek yönetimi, çoklu görev (multitasking) ve kullanıcı modu izolasyonu gibi temel işletim sistemi kavramlarını öğrenmek ve uygulamak amacıyla geliştirilmiştir.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Arch](https://img.shields.io/badge/architecture-x86_i386-orange.svg)
![Status](https://img.shields.io/badge/status-Phase_3_Complete-green.svg)

## 🚀 Öne Çıkan Özellikler

### 🛡️ Güvenlik ve İzolasyon
- **Kullanıcı Modu (Ring 3):** Uygulamaların çekirdekten izole bir şekilde en düşük yetki seviyesinde çalışması.
- **TSS (Task State Segment):** Güvenli donanım bağlam değişimi ve interrupt yönetimi.
- **Syscalls (int 0x80):** Uygulamaların çekirdek servislerine erişimi için güvenli bir köprü.

### 🧠 Bellek Yönetimi
- **Sayfalama (Paging):** 32-bit sanal bellek desteği ve Higher Half Kernel (3GB+) mimarisi.
- **PMM (Physical Memory Manager):** Bitmap tabanlı fiziksel sayfa yönetimi.
- **Kernel Heap:** `kmalloc` ve `kfree` ile dinamik bellek tahsisi.

### 🔄 Çoklu Görev (Multitasking)
- **Round Robin Zamanlayıcı:** Görevler arasında adil işlemci paylaşımı.
- **Context Switching:** Assembly seviyesinde hızlı görev değişimi.

### 📁 Depolama ve Dosya Sistemi
- **FAFS (PekerOS Advanced File System):** Kendi özel dosya sistemi mimarimiz.
- **ATA/IDE Sürücüsü:** Gerçek sabit disk okuma ve yazma desteği.

## 🛠️ Kurulum ve Derleme

### Gereksinimler
Sistemi derlemek ve çalıştırmak için aşağıdaki araçlara ihtiyacınız vardır:
- `gcc` (i386-elf-gcc önerilir)
- `nasm` (Assembly derleyici)
- `ld` (Linker)
- `qemu-system-i386` (Simülasyon için)

### Derleme
```bash
bash scripts/compile.sh
```

### Çalıştırma
```bash
bash scripts/run.sh
```

## 🗺️ Yol Haritası
- [x] **Faz 1:** Bellek Yönetimi (PMM, Paging, Heap)
- [x] **Faz 2:** Depolama (ATA, FAFS, Shell)
- [x] **Faz 3:** Çoklu Görev & Syscalls
- [ ] **Faz 4:** Donanım Keşfi (PCI, VESA Grafik Modu)
- [ ] **Faz 5:** Kullanıcı Uygulamaları (ELF Loader, LibC)

## 📜 Lisans
Bu proje **MIT Lisansı** altında lisanslanmıştır. Detaylar için `LICENSE` dosyasına bakabilirsiniz.

---
*Geliştiren: PekerOS Ekibi & Antigravity AI*
