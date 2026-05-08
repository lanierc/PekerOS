#include "net.h"
#include "rtl8139.h"
#include "kheap.h"

extern void put_str(const char* str);
extern void put_hex(unsigned int n);
extern void put_int(int n);

unsigned short calculate_checksum(void *addr, int count);

// İşletim sistemimizin bilgileri
static unsigned char my_mac[MAC_LEN];
static unsigned char my_ip[IP_LEN] = {10, 0, 2, 15}; // QEMU standart IP
static unsigned char broadcast_mac[MAC_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static unsigned char gateway_mac[MAC_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Gecici ARP cache (son gorulen MAC)

static struct udp_socket udp_sockets[MAX_UDP_SOCKETS];

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
    for (int i = 0; i < MAX_UDP_SOCKETS; i++) udp_sockets[i].in_use = 0;
    put_str("[NET] Ag katmani baslatildi. OS IP: 10.0.2.15\n");
}

void net_send_packet(void *data, int len) {
    rtl8139_send_packet(data, len);
}

int udp_bind(unsigned short port, udp_callback_t callback) {
    for (int i = 0; i < MAX_UDP_SOCKETS; i++) {
        if (!udp_sockets[i].in_use) {
            udp_sockets[i].local_port = port;
            udp_sockets[i].callback = callback;
            udp_sockets[i].in_use = 1;
            return 0; // Success
        }
    }
    return -1; // No free sockets
}

static unsigned short ipv4_id_counter = 0;

void net_send_ipv4(unsigned char *dest_ip, unsigned char protocol, void *payload, int payload_len) {
    int total_len = sizeof(struct eth_header) + sizeof(struct ipv4_header) + payload_len;
    
    unsigned char *buffer = (unsigned char *)kmalloc(total_len);
    if (!buffer) return;

    struct eth_header *eth = (struct eth_header *)buffer;
    memcpy(eth->dest_mac, gateway_mac, MAC_LEN); // Gecici cozum: son gorulen MAC (genelde gateway)
    memcpy(eth->src_mac, my_mac, MAC_LEN);
    eth->eth_type = htons(ETH_TYPE_IPV4);

    struct ipv4_header *ip = (struct ipv4_header *)(buffer + sizeof(struct eth_header));
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->total_len = htons(sizeof(struct ipv4_header) + payload_len);
    ip->id = htons(ipv4_id_counter++);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    memcpy(ip->src_ip, my_ip, IP_LEN);
    memcpy(ip->dest_ip, dest_ip, IP_LEN);
    
    ip->checksum = calculate_checksum(ip, sizeof(struct ipv4_header));

    memcpy(buffer + sizeof(struct eth_header) + sizeof(struct ipv4_header), payload, payload_len);

    net_send_packet(buffer, total_len);
    kfree(buffer);
}

void net_send_udp(unsigned char *dest_ip, unsigned short dest_port, unsigned short src_port, void *payload, int payload_len) {
    int udp_len = sizeof(struct udp_header) + payload_len;
    unsigned char *buffer = (unsigned char *)kmalloc(udp_len);
    if (!buffer) return;

    struct udp_header *udp = (struct udp_header *)buffer;
    udp->src_port = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons(udp_len);
    udp->checksum = 0; // Opsiyonel (0 = hesaplanmadi)

    memcpy(buffer + sizeof(struct udp_header), payload, payload_len);

    net_send_ipv4(dest_ip, 17, buffer, udp_len);
    kfree(buffer);
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

void net_handle_udp(struct eth_header *eth, struct ipv4_header *ip, struct udp_header *udp, int udp_len) {
    unsigned short dest_port = ntohs(udp->dest_port);
    unsigned short src_port = ntohs(udp->src_port);
    int payload_len = ntohs(udp->length) - sizeof(struct udp_header);
    void *payload = (void *)((unsigned char *)udp + sizeof(struct udp_header));

    for (int i = 0; i < MAX_UDP_SOCKETS; i++) {
        if (udp_sockets[i].in_use && udp_sockets[i].local_port == dest_port) {
            if (udp_sockets[i].callback) {
                udp_sockets[i].callback(payload, payload_len, ip->src_ip, src_port);
            }
            break;
        }
    }
}

void net_handle_ipv4(struct eth_header *eth, struct ipv4_header *ip) {
    // Gonderenin MAC adresini kaydet ki ona geri donebilelim
    memcpy(gateway_mac, eth->src_mac, MAC_LEN);

    // Bize mi geliyor?
    if (memcmp(ip->dest_ip, my_ip, IP_LEN) == 0 || memcmp(ip->dest_ip, broadcast_mac, IP_LEN) == 0) { // Bize veya Broadcast (IP broadcast tam dogru degil ama idare eder)
        int header_len = ip->ihl * 4;
        
        if (ip->protocol == 1) { // ICMP
            struct icmp_header *icmp = (struct icmp_header *)((unsigned char *)ip + header_len);
            int icmp_len = ntohs(ip->total_len) - header_len;
            net_handle_icmp(eth, ip, icmp, icmp_len);
        } else if (ip->protocol == 17) { // UDP
            struct udp_header *udp = (struct udp_header *)((unsigned char *)ip + header_len);
            int udp_len = ntohs(ip->total_len) - header_len;
            net_handle_udp(eth, ip, udp, udp_len);
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
