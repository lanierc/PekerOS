#include "rtl8139.h"
#include "idt.h"
#include "pmm.h"
#include "paging.h"

// I/O fonksiyonları (kernel.c'den)
extern void outb(unsigned short port, unsigned char val);
extern unsigned char inb(unsigned short port);
extern void outw(unsigned short port, unsigned short val);
extern unsigned short inw(unsigned short port);
extern void outl(unsigned short port, unsigned int val);
extern unsigned int inl(unsigned short port);

extern void put_str(const char* str);
extern void put_hex(unsigned int n);
extern void put_int(int n);

static unsigned int rtl_io_base = 0;
static unsigned int rx_buffer_phys = 0;
static unsigned int rx_buffer_virt = 0xD0000000;
static unsigned int tx_buffer_phys = 0;
static unsigned int tx_buffer_virt = 0xD0004000;
static unsigned int current_rx_ptr = 0;
static int tx_cur = 0;
static unsigned char mac_address[6];

extern void net_handle_packet(void *packet, int length);
extern void init_net(unsigned char *mac_addr);

void rtl8139_handler(struct registers *r) {
    // Interrupt Status oku
    unsigned short status = inw(rtl_io_base + RTL8139_ISR);
    
    // Status'u temizle (yazdıklarını geri yazar)
    outw(rtl_io_base + RTL8139_ISR, status);

    if (status & 0x01) { // Receive (RX) OK
        // CAPR güncellenmeli.
        // Paket başlığında: [Status (2 bayt)] [Uzunluk (2 bayt)] [Veri...]
        unsigned short *rx = (unsigned short *)(rx_buffer_virt + current_rx_ptr);
        unsigned short packet_length = rx[1];
        
        // Ağ katmanına pasla (ilk 4 byte RTL8139 başlığıdır, CRC de sonda 4 byte'tır)
        void *packet_data = (void *)((unsigned int)rx_buffer_virt + current_rx_ptr + 4);
        int real_length = packet_length - 4; // CRC'yi çıkar
        
        net_handle_packet(packet_data, real_length);
        
        // Sadece okuduk olarak kaydedip CAPR'ı güncelliyoruz. (+4 CRC, +4 başlık)
        current_rx_ptr = (current_rx_ptr + packet_length + 4 + 3) & ~3;
        if (current_rx_ptr > 8192) {
            current_rx_ptr -= 8192;
        }
        
        outw(rtl_io_base + RTL8139_CAPR, current_rx_ptr - 16);
    }
    
    if (status & 0x20) { // Link Change
        put_str("[RTL8139] Link durumu degisti.\n");
    }
}

void init_rtl8139(unsigned int base_addr, unsigned char irq) {
    rtl_io_base = base_addr;
    put_str("\n[RTL8139] Kurulum basliyor. I/O Base: 0x");
    put_hex(rtl_io_base);
    put_str(" IRQ: ");
    put_int(irq);
    put_str("\n");

    // 1. Cihazı aç (Config1 register)
    outb(rtl_io_base + RTL8139_CONFIG1, 0x00);

    // 2. Yazılım Sıfırlaması (Software Reset)
    outb(rtl_io_base + RTL8139_CR, 0x10);
    while((inb(rtl_io_base + RTL8139_CR) & 0x10) != 0) {
        // Reset bitinin sıfırlanmasını bekle
    }

    // 3. RX ve TX Buffer oluştur (RX: ~12KB = 3 frame, TX: ~8KB = 2 frame)
    rx_buffer_phys = alloc_contiguous_frames(3);
    paging_map_memory(rx_buffer_phys, rx_buffer_virt, 3 * 4096);
    
    tx_buffer_phys = alloc_contiguous_frames(2);
    paging_map_memory(tx_buffer_phys, tx_buffer_virt, 2 * 4096);
    
    // RX Buffer fiziksel adresini karta ver
    outl(rtl_io_base + RTL8139_RBSTART, rx_buffer_phys);

    // 4. Kesmeleri ayarla (Interrupt Mask Register - IMR)
    // Sadece Receive (RX) OK ve Link Change (0x01 | 0x20) => 0x0005
    // Biz her şeyi almak için 0x0005 yapıyoruz.
    outw(rtl_io_base + RTL8139_IMR, 0x0005);

    // 5. RX Konfigürasyonunu (RCR) ayarla (Promiscuous, Broadcast, Multicast vs)
    // AB+AM+APM+AAP (Kabul et her şeyi) => 0xF
    // WRAP bit (Buffer sonuna gelince sarmalama) => (1 << 7)
    outl(rtl_io_base + RTL8139_RCR, 0xF | (1 << 7));

    // 6. RX ve TX'i aktifleştir
    outb(rtl_io_base + RTL8139_CR, 0x0C); // RE (Receive Enable) + TE (Transmit Enable)

    // MAC Adresini Oku
    for (int i = 0; i < 6; i++) {
        mac_address[i] = inb(rtl_io_base + RTL8139_MAC05 + i);
    }
    
    put_str("[RTL8139] MAC Adresi: ");
    for(int i=0; i<6; i++) {
        put_hex(mac_address[i]);
        if(i < 5) put_str(":");
    }
    put_str("\n");

    init_net(mac_address);

    // IRQ Kaydı
    irq_install_handler(irq, rtl8139_handler);

    // Master/Slave PIC Mask temizliği (Eğer IRQ > 7 ise slave de açılmalı)
    if (irq < 8) {
        unsigned char mask = inb(0x21);
        outb(0x21, mask & ~(1 << irq));
    } else {
        unsigned char mask = inb(0xA1);
        outb(0xA1, mask & ~(1 << (irq - 8)));
    }
    
    put_str("[OK] RTL8139 Basariyla yuklendi!\n");
}

void rtl8139_send_packet(void *data, int len) {
    if (len > 1792) return; // Desteklenen maksimum frame boyutu (RTL8139 için güvenli)

    // TX Buffer'a kopyala
    void *tx_buf = (void *)(tx_buffer_virt + (tx_cur * 2048));
    memcpy(tx_buf, data, len);

    // TX Address ve Status register'larına yaz
    outl(rtl_io_base + RTL8139_TSAD0 + (tx_cur * 4), tx_buffer_phys + (tx_cur * 2048));
    outl(rtl_io_base + RTL8139_TSD0 + (tx_cur * 4), len); // Uzunluğu yazınca gönderim tetiklenir

    tx_cur = (tx_cur + 1) % 4; // 4 TX descriptor var
}
