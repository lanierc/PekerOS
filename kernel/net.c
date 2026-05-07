#include "net.h"
#include "rtl8139.h"
#include "kheap.h"

extern void put_str(const char* str);
extern void put_hex(unsigned int n);
extern void put_int(int n);
// İşletim sistemimizin bilgileri
static unsigned char my_mac[MAC_LEN];
static unsigned char my_ip[IP_LEN] = {10, 0, 2, 15}; // QEMU standart IP
static unsigned char broadcast_mac[MAC_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

int memcmp(const void *s1, const void *s2, int n) {
    const unsigned char *p1 = s1, *p2 = s2;
    while(n--) {
        if(*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

void init_net(unsigned char *mac_addr) {
    memcpy(my_mac, mac_addr, MAC_LEN);
    put_str("[NET] Ag katmani baslatildi. OS IP: 10.0.2.15\n");
}

void net_send_packet(void *data, int len) {
    rtl8139_send_packet(data, len);
}

// Basit Checksum Hesaplama (RFC 1071)
unsigned short calculate_checksum(void *addr, int count) {
    register unsigned int sum = 0;
    unsigned short *ptr = (unsigned short *)addr;

    while (count > 1) {
        sum += *ptr++;
        count -= 2;
    }

    if (count > 0) {
        sum += *(unsigned char *)ptr;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

void net_handle_arp(struct eth_header *eth, struct arp_packet *arp) {
    if (ntohs(arp->opcode) == 1) { // ARP Request
        // Bize mi soruyorlar?
        if (memcmp(arp->target_ip, my_ip, IP_LEN) == 0) {
            put_str("[NET] ARP İstegi alindi! (Bizi ariyorlar). Cevap gonderiliyor...\n");
            
            // Cevap paketini hazırlayalım
            // Mevcut paketin üzerine yazıp direkt geri yollamak en kolayıdır.
            
            // Ethernet başlığı
            memcpy(eth->dest_mac, eth->src_mac, MAC_LEN);
            memcpy(eth->src_mac, my_mac, MAC_LEN);
            
            // ARP verisi
            arp->opcode = htons(2); // Reply
            
            memcpy(arp->target_mac, arp->sender_mac, MAC_LEN);
            memcpy(arp->target_ip, arp->sender_ip, IP_LEN);
            
            memcpy(arp->sender_mac, my_mac, MAC_LEN);
            memcpy(arp->sender_ip, my_ip, IP_LEN);
            
            // Gönder
            net_send_packet(eth, sizeof(struct eth_header) + sizeof(struct arp_packet));
        }
    }
}

void net_handle_icmp(struct eth_header *eth, struct ipv4_header *ip, struct icmp_header *icmp, int icmp_len) {
    if (icmp->type == 8) { // Echo Request (Ping)
        put_str("[NET] ICMP Ping İstegi Alindi! Cevap gonderiliyor...\n");
        
        // Ethernet başlığını ters çevir
        memcpy(eth->dest_mac, eth->src_mac, MAC_LEN);
        memcpy(eth->src_mac, my_mac, MAC_LEN);
        
        // IPv4 başlığını ters çevir
        unsigned char temp_ip[IP_LEN];
        memcpy(temp_ip, ip->dest_ip, IP_LEN);
        memcpy(ip->dest_ip, ip->src_ip, IP_LEN);
        memcpy(ip->src_ip, temp_ip, IP_LEN);
        
        // IP checksum güncelle
        ip->checksum = 0;
        ip->checksum = calculate_checksum(ip, (ip->ihl * 4));
        
        // ICMP paketini Reply yap
        icmp->type = 0; // Echo Reply
        icmp->checksum = 0;
        icmp->checksum = calculate_checksum(icmp, icmp_len);
        
        int total_len = sizeof(struct eth_header) + ntohs(ip->total_len);
        net_send_packet(eth, total_len);
    }
}

void net_handle_ipv4(struct eth_header *eth, struct ipv4_header *ip) {
    // Bize mi geliyor?
    if (memcmp(ip->dest_ip, my_ip, IP_LEN) == 0 || memcmp(ip->dest_ip, broadcast_mac, IP_LEN) == 0) { // Bize veya Broadcast (IP broadcast tam dogru degil ama idare eder)
        int header_len = ip->ihl * 4;
        
        if (ip->protocol == 1) { // ICMP
            struct icmp_header *icmp = (struct icmp_header *)((unsigned char *)ip + header_len);
            int icmp_len = ntohs(ip->total_len) - header_len;
            net_handle_icmp(eth, ip, icmp, icmp_len);
        }
    }
}

void net_handle_packet(void *packet, int length) {
    if (length < sizeof(struct eth_header)) return;
    
    struct eth_header *eth = (struct eth_header *)packet;
    unsigned short type = ntohs(eth->eth_type);
    
    if (type == ETH_TYPE_ARP) {
        struct arp_packet *arp = (struct arp_packet *)((unsigned char *)packet + sizeof(struct eth_header));
        net_handle_arp(eth, arp);
    } else if (type == ETH_TYPE_IPV4) {
        struct ipv4_header *ip = (struct ipv4_header *)((unsigned char *)packet + sizeof(struct eth_header));
        net_handle_ipv4(eth, ip);
    }
}
