#ifndef NET_H
#define NET_H

#include "common.h"

// Endianness Dönüştürme (x86 Little Endian -> Network Big Endian)
static inline unsigned short htons(unsigned short v) {
    return (v >> 8) | (v << 8);
}
static inline unsigned short ntohs(unsigned short v) {
    return htons(v);
}
static inline unsigned int htonl(unsigned int v) {
    return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v >> 8) & 0xFF00) | ((v >> 24) & 0xFF);
}
static inline unsigned int ntohl(unsigned int v) {
    return htonl(v);
}

// MAC Adresi Uzunluğu
#define MAC_LEN 6
#define IP_LEN 4

// Protokol Tipleri
#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IPV4 0x0800

#pragma pack(push, 1)

// Ethernet Başlığı
struct eth_header {
    unsigned char dest_mac[MAC_LEN];
    unsigned char src_mac[MAC_LEN];
    unsigned short eth_type;
};

// ARP Paketi
struct arp_packet {
    unsigned short hw_type;    // 1 (Ethernet)
    unsigned short proto_type; // 0x0800 (IPv4)
    unsigned char hw_len;      // 6
    unsigned char proto_len;   // 4
    unsigned short opcode;     // 1: Request, 2: Reply
    unsigned char sender_mac[MAC_LEN];
    unsigned char sender_ip[IP_LEN];
    unsigned char target_mac[MAC_LEN];
    unsigned char target_ip[IP_LEN];
};

// IPv4 Başlığı
struct ipv4_header {
    unsigned char ihl : 4;
    unsigned char version : 4;
    unsigned char tos;
    unsigned short total_len;
    unsigned short id;
    unsigned short flags_frag;
    unsigned char ttl;
    unsigned char protocol;    // 1: ICMP, 6: TCP, 17: UDP
    unsigned short checksum;
    unsigned char src_ip[IP_LEN];
    unsigned char dest_ip[IP_LEN];
};

// ICMP Başlığı
struct icmp_header {
    unsigned char type;        // 8: Echo Request, 0: Echo Reply
    unsigned char code;
    unsigned short checksum;
    unsigned short identifier;
    unsigned short sequence;
};

#pragma pack(pop)

// Fonksiyonlar
void init_net(unsigned char *mac_addr);
void net_handle_packet(void *packet, int length);
void net_send_packet(void *data, int len);

#endif
