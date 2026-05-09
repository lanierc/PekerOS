#ifndef TCP_H
#define TCP_H

#include "common.h"

struct ipv4_header;
struct tcp_header;

// TCP Durumları (States)
typedef enum {
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT
} tcp_state_t;

// TCP Bayrakları (Flags)
#define TCP_FIN  (1 << 0)
#define TCP_SYN  (1 << 1)
#define TCP_RST  (1 << 2)
#define TCP_PSH  (1 << 3)
#define TCP_ACK  (1 << 4)
#define TCP_URG  (1 << 5)

#pragma pack(push, 1)
struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  res1 : 4;
    uint8_t  data_offset : 4;
    uint8_t  flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
};
#pragma pack(pop)

// TCP Protocol Control Block (PCB)
typedef struct tcp_pcb {
    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    
    tcp_state_t state;
    
    uint32_t snd_nxt; // Sırada gönderilecek seq num
    uint32_t rcv_nxt; // Beklenen ack num
    
    uint16_t window_size;
    
    void *socket; // Bağlı olduğu socket_t göstericisi
} tcp_pcb_t;

void tcp_input(struct ipv4_header *ip, struct tcp_header *tcp, uint16_t len);
void tcp_init();
void tcp_close(tcp_pcb_t *pcb);
uint16_t tcp_get_free_port();

#endif
