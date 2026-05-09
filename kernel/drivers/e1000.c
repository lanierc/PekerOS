#include "common.h"
#include "e1000.h"
#include "idt.h"
#include "pmm.h"
#include "paging.h"
#include "net.h"
#include "kheap.h"

extern void put_str(const char* str);
extern void put_hex(unsigned int n);
extern void put_int(int n);
extern void irq_install_handler(int irq, void (*handler)(struct registers *));
extern uint32_t alloc_contiguous_frames(int n);

static uint32_t mmio_base_virt = 0xD0010000; 
static unsigned char e1000_mac[6];

// Descriptors
static struct e1000_rx_desc *rx_descs;
static struct e1000_tx_desc *tx_descs;
static uint32_t rx_descs_phys;
static uint32_t tx_descs_phys;

// Buffers
static uint8_t *rx_buffers;
static uint32_t rx_buffers_phys;

static uint16_t rx_cur = 0;
static uint16_t tx_cur = 0;

void e1000_write(uint16_t address, uint32_t value) {
    (*(volatile uint32_t*)(mmio_base_virt + address)) = value;
}

uint32_t e1000_read(uint16_t address) {
    return (*(volatile uint32_t*)(mmio_base_virt + address));
}

void e1000_handler(struct registers *r) {
    uint32_t status = e1000_read(E1000_ICR);
    
    if (status & 0x80) { // Receiver Timer Interrupt (Packet received)
        while (rx_descs[rx_cur].status & 0x01) { // DD (Descriptor Done)
            uint16_t len = rx_descs[rx_cur].length;
            void *packet_data = (void *)((uint32_t)rx_buffers + (rx_cur * 2048));
            
            net_handle_packet(packet_data, len);
            
            rx_descs[rx_cur].status = 0;
            uint16_t old_cur = rx_cur;
            rx_cur = (rx_cur + 1) % E1000_NUM_RX_DESC;
            e1000_write(E1000_RDT, old_cur); 
        }
    }
}

void e1000_read_mac() {
    uint32_t low = e1000_read(E1000_RAL);
    uint32_t high = e1000_read(E1000_RAH);
    
    e1000_mac[0] = low & 0xFF;
    e1000_mac[1] = (low >> 8) & 0xFF;
    e1000_mac[2] = (low >> 16) & 0xFF;
    e1000_mac[3] = (low >> 24) & 0xFF;
    e1000_mac[4] = high & 0xFF;
    e1000_mac[5] = (high >> 8) & 0xFF;
}

void e1000_send_packet(void *data, int len) {
    static uint8_t *tx_buffers = 0;
    static uint32_t tx_buffers_phys = 0;
    
    if(!tx_buffers) {
        tx_buffers_phys = alloc_contiguous_frames(64); // 128KB
        tx_buffers = (uint8_t *)0xD0060000;
        paging_map_memory(tx_buffers_phys, (uint32_t)tx_buffers, 128*1024);
    }

    memcpy(tx_buffers + (tx_cur * 2048), data, len);
    tx_descs[tx_cur].addr = tx_buffers_phys + (tx_cur * 2048);
    tx_descs[tx_cur].length = len;
    tx_descs[tx_cur].cmd = (1 << 0) | (1 << 1); // EOP | IFCS
    tx_descs[tx_cur].status = 0;

    uint16_t old_cur = tx_cur;
    tx_cur = (tx_cur + 1) % E1000_NUM_TX_DESC;
    e1000_write(E1000_TDT, tx_cur);

    while(!(tx_descs[old_cur].status & 0x01)); 
}

void init_e1000(unsigned int phys_addr, unsigned char irq) {
    put_str("[E1000] Kurulum basliyor. MMIO: 0x");
    put_hex(phys_addr);
    put_str(" IRQ: ");
    put_int(irq);
    put_str("\n");

    paging_map_memory(phys_addr, mmio_base_virt, 128 * 1024);

    e1000_write(E1000_CTRL, E1000_CTRL_RST);
    for(int i=0; i<10000; i++) asm volatile("nop"); 

    uint32_t ctrl = e1000_read(E1000_CTRL);
    ctrl |= E1000_CTRL_SLU | E1000_CTRL_ASDE;
    e1000_write(E1000_CTRL, ctrl);

    e1000_read_mac();
    put_str("[E1000] MAC Adresi: ");
    for(int i=0; i<6; i++) {
        put_hex(e1000_mac[i]);
        if(i < 5) put_str(":");
    }
    put_str("\n");

    rx_descs_phys = alloc_contiguous_frames(1); 
    rx_descs = (struct e1000_rx_desc *)(0xD0020000);
    paging_map_memory(rx_descs_phys, (uint32_t)rx_descs, 4096);
    
    rx_buffers_phys = alloc_contiguous_frames((E1000_NUM_RX_DESC * 2048) / 4096);
    rx_buffers = (uint8_t *)(0xD0030000);
    paging_map_memory(rx_buffers_phys, (uint32_t)rx_buffers, E1000_NUM_RX_DESC * 2048);

    for(int i=0; i<E1000_NUM_RX_DESC; i++) {
        rx_descs[i].addr = rx_buffers_phys + (i * 2048);
        rx_descs[i].status = 0;
    }

    e1000_write(E1000_RDBAL, rx_descs_phys);
    e1000_write(E1000_RDBAH, 0);
    e1000_write(E1000_RDLEN, E1000_NUM_RX_DESC * sizeof(struct e1000_rx_desc));
    e1000_write(E1000_RDH, 0);
    e1000_write(E1000_RDT, E1000_NUM_RX_DESC - 1);
    e1000_write(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_SBP | E1000_RCTL_BAM | E1000_RCTL_SECRC);

    tx_descs_phys = alloc_contiguous_frames(1);
    tx_descs = (struct e1000_tx_desc *)(0xD0040000);
    paging_map_memory(tx_descs_phys, (uint32_t)tx_descs, 4096);

    for(int i=0; i<E1000_NUM_TX_DESC; i++) {
        tx_descs[i].status = 0x01; 
    }

    e1000_write(E1000_TDBAL, tx_descs_phys);
    e1000_write(E1000_TDBAH, 0);
    e1000_write(E1000_TDLEN, E1000_NUM_TX_DESC * sizeof(struct e1000_tx_desc));
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);
    e1000_write(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP);

    irq_install_handler(irq, e1000_handler);
    e1000_write(E1000_IMS, 0x1F6DC); 
    e1000_read(E1000_ICR); 

    init_net(e1000_mac);
    net_register_driver(e1000_send_packet);
    put_str("[OK] Intel E1000 Basariyla yuklendi!\n");
}