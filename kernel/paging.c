#include "paging.h"
#include "pmm.h"

extern unsigned int __text_start;
extern unsigned int __text_end;
extern unsigned int __rodata_start;
extern unsigned int __rodata_end;

// Kernel alanında tutulacak page directory
// Her ikisinin de 4KB hizalı (aligned) olması önemlidir (Paging için kural).
// Bu nedenle frame alıp oraya yerleştireceğiz veya hizalı global kullanacağız.
// Şimdilik PMM'den frame alıp onun üzerine yazacağız.

static struct page_directory *kernel_directory;

// Assembly fonksiyonları (boot.s'ten gelecek)
extern void load_page_directory(unsigned int cr3_value);
extern void enable_paging();

void init_paging(void) {
    // 1. Page Directory için bir frame (4KB) ayır
    unsigned int pd_frame = alloc_frame();
    // Sayfalama boot.s'te açıldığı için PMM'den dönen fiziksel adresi sanala çeviriyoruz
    kernel_directory = (struct page_directory *)(pd_frame + 0xC0000000);

    // Tüm PDE'leri temizle
    for (int i = 0; i < 1024; i++) {
        kernel_directory->entries[i].present = 0;
        kernel_directory->entries[i].rw = 1;
        kernel_directory->entries[i].user = 0;
        kernel_directory->entries[i].pt_frame = 0;
    }

    // 2. Identity Mapping: Sistemdeki ilk belleği haritala (Örn: İlk 16 MB veya mevcut tüm RAM)
    // Şimdilik PMM'den dönen bellek sınırına kadar identity mapping yapıyoruz.
    unsigned int total_kb = pmm_total_memory_kb();
    unsigned int total_frames = (total_kb * 1024) / PAGE_SIZE;
    
    // Kaç tane Page Table gerekecek? (Her biri 1024 frame = 4MB kapsar)
    unsigned int num_tables = (total_frames + 1023) / 1024;
    
    for (unsigned int i = 0; i < num_tables; i++) {
        unsigned int pt_frame = alloc_frame();
        // Page Table fiziksel adresini sanal adrese çeviriyoruz
        struct page_table *pt = (struct page_table *)(pt_frame + 0xC0000000);

        for (int j = 0; j < 1024; j++) {
            unsigned int frame_idx = (i * 1024) + j;
            if (frame_idx < total_frames) {
                unsigned int phys_addr = frame_idx * PAGE_SIZE;
                int is_readonly = 0;

                // Linker sembolleri artık sanal adres (0xC0100000+).
                // Karşılaştırmak için sanal adres oluşturuyoruz.
                unsigned int virt_addr = phys_addr + 0xC0000000;

                // .text (kod) ve .rodata (sabit veriler) alanları yazmaya karşı korunur
                if (virt_addr >= (unsigned int)&__text_start && virt_addr < (unsigned int)&__text_end) {
                    is_readonly = 1;
                }
                if (virt_addr >= (unsigned int)&__rodata_start && virt_addr < (unsigned int)&__rodata_end) {
                    is_readonly = 1;
                }

                pt->entries[j].present = 1;
                pt->entries[j].rw = is_readonly ? 0 : 1;
                pt->entries[j].user = 1; // Ring 3 erişimine izin ver
                pt->entries[j].frame = frame_idx;
            } else {
                pt->entries[j].present = 0;
            }
        }

        // Higher Half Mapping: 0xC0000000 adresinden itibaren (Entry 768)
        // 768 + i kullanıyoruz ki fiziksel 0, sanal 3GB'a denk gelsin
        kernel_directory->entries[768 + i].present = 1;
        kernel_directory->entries[768 + i].rw = 1;
        kernel_directory->entries[768 + i].user = 1; // Ring 3 erişimine izin ver
        kernel_directory->entries[768 + i].pt_frame = (pt_frame >> 12);
    }

    // 3. ISR 14 Handler kaydı
    irq_install_handler(14, page_fault);

    // 4. CR3 register'ını yükle. (CR3 fiziksel adres ister!)
    // Bu aşamada boot.s'teki Identity Mapping yok olacak. Artık sadece Higher Half var.
    load_page_directory(pd_frame);
    // Paging zaten açık, enable_paging() çağırmaya gerek yok (veya çağırmak zararsızdır).


    put_str("[OK] Paging (Sanal Bellek) aktifleştirildi.\n");
}

void page_fault(struct registers *r) {
    // Fault olan adresi CR2 register'ından oku
    unsigned int faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    int present   = !(r->err_code & 0x1); // Sayfa yok (not present)
    int rw        = r->err_code & 0x2;    // Sadece okuma sayfasında yazma hatası
    int us        = r->err_code & 0x4;    // Kullanıcı modunda (user-mode)
    int reserved  = r->err_code & 0x8;    // Ayrılmış bitlerin üzerine yazıldı
    int id        = r->err_code & 0x10;   // Instruction fetch yüzünden kaynaklandı

    put_str("\n!!! Page Fault !!!\nAdres: 0x");
    put_hex(faulting_address);
    put_str("\nHata Tipi: ");
    if (present) { put_str("Sayfa Bulunamadi (Not Present) "); }
    if (rw) { put_str("Salt Okunur Ihlali (Read-Only) "); }
    if (us) { put_str("Kullanici Hakki Ihlali (User Mode) "); }
    if (reserved) { put_str("Ayrilmis Bit İhlali (Reserved) "); }
    if (id) { put_str("Talimat Getirme Hatasi (Instruction Fetch) "); }

    put_str("\nSistem Durduruldu.\n");
    asm volatile("cli");
    for(;;) asm volatile("hlt");
}
