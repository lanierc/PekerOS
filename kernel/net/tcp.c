#include "tcp.h"
#include "net.h"
#include "kheap.h"
#include "socket.h"

extern void put_str(const char* str);
extern void put_int(int n);

#define MAX_TCP_PCBS 16
static tcp_pcb_t *tcp_pcbs[MAX_TCP_PCBS];

void tcp_init() {
    for (int i = 0; i < MAX_TCP_PCBS; i++) tcp_pcbs[i] = 0;
}

tcp_pcb_t *tcp_new_pcb() {
    for (int i = 0; i < MAX_TCP_PCBS; i++) {
        if (tcp_pcbs[i] == 0) {
            tcp_pcb_t *pcb = (tcp_pcb_t *)kmalloc(sizeof(tcp_pcb_t));
            memset(pcb, 0, sizeof(tcp_pcb_t));
            pcb->state = TCP_CLOSED;
            pcb->window_size = 4096;
            tcp_pcbs[i] = pcb;
            return pcb;
        }
    }
    return 0;
}

// TCP Checksum hesaplama (Pseudo-header dahil)
uint16_t tcp_checksum(struct ipv4_header *ip, struct tcp_header *tcp, uint16_t len) {
    uint32_t sum = 0;
    uint16_t *p;
    
    // Pseudo-header
    p = (uint16_t *)ip->src_ip;
    sum += *p++; sum += *p++;
    p = (uint16_t *)ip->dest_ip;
    sum += *p++; sum += *p++;
    
    sum += htons(ip->protocol);
    sum += htons(len);
    
    // Header + Data
    p = (uint16_t *)tcp;
    int count = len;
    while (count > 1) {
        sum += *p++;
        count -= 2;
    }
    if (count > 0) {
        sum += *(uint8_t *)p;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~((uint16_t)sum);
}

void tcp_send_segment(tcp_pcb_t *pcb, uint8_t flags, uint8_t *data, uint16_t data_len) {
    uint16_t total_len = sizeof(struct tcp_header) + data_len;
    uint8_t *buffer = (uint8_t *)kmalloc(total_len);
    
    struct tcp_header *tcp = (struct tcp_header *)buffer;
    tcp->src_port = htons(pcb->local_port);
    tcp->dest_port = htons(pcb->remote_port);
    tcp->seq_num = htonl(pcb->snd_nxt);
    tcp->ack_num = htonl(pcb->rcv_nxt);
    tcp->data_offset = 5; // 20 bytes
    tcp->res1 = 0;
    tcp->flags = flags;
    tcp->window_size = htons(pcb->window_size);
    tcp->checksum = 0;
    tcp->urgent_ptr = 0;
    
    if (data_len > 0) {
        memcpy(buffer + sizeof(struct tcp_header), data, data_len);
    }
    
    // Checksum icin gecici bir IP yapisi lazim (Pseudo-header icin)
    struct ipv4_header ip_pseudo;
    memcpy(ip_pseudo.src_ip, &pcb->local_ip, 4);
    memcpy(ip_pseudo.dest_ip, &pcb->remote_ip, 4);
    ip_pseudo.protocol = 6;
    
    tcp->checksum = tcp_checksum(&ip_pseudo, tcp, total_len);
    
    net_send_ipv4((uint8_t *)&pcb->remote_ip, 6, buffer, total_len);
    
    // Seq num guncelle (SYN ve FIN 1 byte sayilir)
    if (flags & (TCP_SYN | TCP_FIN)) pcb->snd_nxt++;
    pcb->snd_nxt += data_len;
    
    kfree(buffer);
}

uint16_t tcp_get_free_port() {
    static uint16_t next_port = 10000;
    uint16_t port = next_port++;
    if (next_port > 60000) next_port = 10000;
    return port;
}

void tcp_close(tcp_pcb_t *pcb) {
    if (!pcb) return;
    
    if (pcb->state == TCP_ESTABLISHED) {
        put_str("[TCP] Kapatma baslatiliyor. FIN gonderiliyor...\n");
        tcp_send_segment(pcb, TCP_FIN | TCP_ACK, 0, 0);
        pcb->state = TCP_FIN_WAIT_1;
    } else if (pcb->state == TCP_CLOSE_WAIT) {
        put_str("[TCP] CLOSE_WAIT -> FIN gonderiliyor (Last ACK).\n");
        tcp_send_segment(pcb, TCP_FIN | TCP_ACK, 0, 0);
        pcb->state = TCP_LAST_ACK;
    } else {
        pcb->state = TCP_CLOSED;
    }
}

void tcp_input(struct ipv4_header *ip, struct tcp_header *tcp, uint16_t len) {
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    uint32_t seq = ntohl(tcp->seq_num);
    uint32_t ack = ntohl(tcp->ack_num);
    
    // Ilgili PCB'yi bul
    tcp_pcb_t *pcb = 0;
    int pcb_index = -1;
    for (int i = 0; i < MAX_TCP_PCBS; i++) {
        if (tcp_pcbs[i] && tcp_pcbs[i]->local_port == dest_port) {
            if (tcp_pcbs[i]->state == TCP_LISTEN || 
                (tcp_pcbs[i]->remote_port == src_port && memcmp(&tcp_pcbs[i]->remote_ip, ip->src_ip, 4) == 0)) {
                pcb = tcp_pcbs[i];
                pcb_index = i;
                break;
            }
        }
    }
    
    if (!pcb) return;
    
    // State Machine
    switch (pcb->state) {
        case TCP_LISTEN:
            if (tcp->flags & TCP_SYN) {
                pcb->remote_port = src_port;
                memcpy(&pcb->remote_ip, ip->src_ip, 4);
                pcb->rcv_nxt = seq + 1;
                pcb->state = TCP_SYN_RECEIVED;
                tcp_send_segment(pcb, TCP_SYN | TCP_ACK, 0, 0);
            }
            break;
            
        case TCP_SYN_SENT:
            if ((tcp->flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
                pcb->rcv_nxt = seq + 1;
                pcb->state = TCP_ESTABLISHED;
                tcp_send_segment(pcb, TCP_ACK, 0, 0);
                
                if (pcb->socket) {
                    socket_t *s = (socket_t *)pcb->socket;
                    if (s->waiting_task) {
                        s->waiting_task->state = TASK_READY;
                        s->waiting_task = 0;
                    }
                }
            }
            break;
            
        case TCP_SYN_RECEIVED:
            if (tcp->flags & TCP_ACK) {
                pcb->state = TCP_ESTABLISHED;
            }
            break;
            
        case TCP_ESTABLISHED:
            if (tcp->flags & TCP_FIN) {
                pcb->rcv_nxt = seq + 1;
                tcp_send_segment(pcb, TCP_ACK, 0, 0);
                pcb->state = TCP_CLOSE_WAIT;
                // Wake up recv() to see EOF
                if (pcb->socket) {
                    socket_t *s = (socket_t *)pcb->socket;
                    if (s->waiting_task) {
                        s->waiting_task->state = TASK_READY;
                        s->waiting_task = 0;
                    }
                }
            } else if (len > (tcp->data_offset * 4)) {
                uint16_t data_len = len - (tcp->data_offset * 4);
                uint8_t *data = (uint8_t *)tcp + (tcp->data_offset * 4);
                
                if (pcb->socket) {
                    socket_t *s = (socket_t *)pcb->socket;
                    if (s->buffer_len + data_len <= SOCKET_BUFFER_SIZE) {
                        for (int i = 0; i < data_len; i++) {
                            s->buffer[s->buffer_head] = data[i];
                            s->buffer_head = (s->buffer_head + 1) % SOCKET_BUFFER_SIZE;
                            s->buffer_len++;
                        }
                        pcb->rcv_nxt = seq + data_len;
                        tcp_send_segment(pcb, TCP_ACK, 0, 0);
                        
                        if (s->waiting_task) {
                            s->waiting_task->state = TASK_READY;
                            s->waiting_task = 0;
                        }
                    }
                }
            }
            break;

        case TCP_FIN_WAIT_1:
            if (tcp->flags & TCP_ACK) {
                pcb->state = TCP_FIN_WAIT_2;
            }
            if (tcp->flags & TCP_FIN) {
                pcb->rcv_nxt = seq + 1;
                tcp_send_segment(pcb, TCP_ACK, 0, 0);
                pcb->state = (pcb->state == TCP_FIN_WAIT_2) ? TCP_TIME_WAIT : TCP_CLOSING;
            }
            break;

        case TCP_FIN_WAIT_2:
            if (tcp->flags & TCP_FIN) {
                pcb->rcv_nxt = seq + 1;
                tcp_send_segment(pcb, TCP_ACK, 0, 0);
                pcb->state = TCP_TIME_WAIT;
                put_str("[TCP] Baglanti kapatildi (Passive Close).\n");
                // In a real OS we wait, here we just close
                pcb->state = TCP_CLOSED;
                kfree(pcb);
                tcp_pcbs[pcb_index] = 0;
            }
            break;

        case TCP_LAST_ACK:
            if (tcp->flags & TCP_ACK) {
                pcb->state = TCP_CLOSED;
                kfree(pcb);
                tcp_pcbs[pcb_index] = 0;
                put_str("[TCP] Baglanti temizlendi.\n");
            }
            break;
            
        default:
            break;
    }
}
