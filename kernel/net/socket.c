#include "socket.h"
#include "kheap.h"
#include "net.h"
#include "paging.h"
#include "spinlock.h"

#define MAX_SOCKETS 32
static socket_t sockets[MAX_SOCKETS];

static uint32_t socket_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    return socket_recvfrom(node, buffer, size, 0, NULL, NULL);
}

static uint32_t socket_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    return socket_sendto(node, buffer, size, 0, NULL, 0);
}

static void socket_close(vfs_node_t *node) {
    socket_t *s = (socket_t *)node->ptr;
    if (s) {
        if (s->type == SOCK_STREAM && s->tcp_pcb) {
            extern void tcp_close(tcp_pcb_t *pcb);
            tcp_close(s->tcp_pcb);
        }
        s->in_use = 0;
        if (s->buffer) kfree(s->buffer);
    }
    kfree(node);
}

vfs_node_t* socket_create(int domain, int type, int protocol) {
    int i;
    for (i = 0; i < MAX_SOCKETS; i++) {
        if (!sockets[i].in_use) break;
    }
    if (i == MAX_SOCKETS) return NULL;

    socket_t *s = &sockets[i];
    s->domain = domain;
    s->type = type;
    s->protocol = protocol;
    s->in_use = 1;
    s->local_port = 0;
    s->buffer = (uint8_t *)kmalloc(SOCKET_BUFFER_SIZE);
    s->buffer_head = 0;
    s->buffer_tail = 0;
    s->buffer_len = 0;
    s->waiting_task = NULL;

    vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));
    node->flags = VFS_SOCKET;
    node->ptr = (struct vfs_node *)s; 
    node->read = socket_read;
    node->write = socket_write;
    node->close = socket_close;

    return node;
}

int socket_bind(vfs_node_t *node, const struct sockaddr *addr, uint32_t addrlen) {
    socket_t *s = (socket_t *)node->ptr;
    struct sockaddr_in *sin = (struct sockaddr_in *)addr;
    s->local_port = ntohs(sin->sin_port);
    s->local_addr = sin->sin_addr.s_addr;

    if (s->type == SOCK_STREAM) {
        tcp_pcb_t *pcb = (tcp_pcb_t *)s->tcp_pcb;
        if (pcb) {
            pcb->local_port = s->local_port;
            pcb->local_ip = s->local_addr;
        }
    }
    return 0;
}

extern tcp_pcb_t *tcp_new_pcb();
extern void tcp_send_segment(tcp_pcb_t *pcb, uint8_t flags, uint8_t *data, uint16_t data_len);

int socket_connect(vfs_node_t *node, const struct sockaddr *addr, uint32_t addrlen) {
    socket_t *s = (socket_t *)node->ptr;
    struct sockaddr_in *sin = (struct sockaddr_in *)addr;
    s->remote_addr = sin->sin_addr.s_addr;
    s->remote_port = ntohs(sin->sin_port);

    if (s->type == SOCK_STREAM) {
        if (!s->tcp_pcb) {
            s->tcp_pcb = tcp_new_pcb();
            ((tcp_pcb_t *)s->tcp_pcb)->socket = s;
        }
        tcp_pcb_t *pcb = (tcp_pcb_t *)s->tcp_pcb;
        extern uint16_t tcp_get_free_port();
        pcb->local_port = tcp_get_free_port();
        pcb->local_ip = net_get_ip();
        pcb->remote_port = s->remote_port;
        memcpy(&pcb->remote_ip, &s->remote_addr, 4);
        
        // Önce ARP ile MAC adresini öğrenmeye çalış (eğer bilmiyorsak)
        extern void net_send_arp_request(unsigned char *target_ip);
        net_send_arp_request((unsigned char *)&s->remote_addr);
        
        // ARP cevabı için kısa bir süre bekle (schedule ile diğer görevlere izin vererek)
        for (int i = 0; i < 100; i++) schedule();

        pcb->state = TCP_SYN_SENT;
        pcb->snd_nxt = 100; // Random ISN
        tcp_send_segment(pcb, TCP_SYN, 0, 0);

        // Bekle (Handshake bitene kadar)
        s->waiting_task = get_current_task();
        s->waiting_task->state = TASK_SLEEP;
        schedule();

        if (pcb->state != TCP_ESTABLISHED) return -1;
    }
    return 0;
}

int socket_listen(vfs_node_t *node, int backlog) {
    socket_t *s = (socket_t *)node->ptr;
    if (s->type == SOCK_STREAM) {
        if (!s->tcp_pcb) {
            s->tcp_pcb = tcp_new_pcb();
            ((tcp_pcb_t *)s->tcp_pcb)->socket = s;
        }
        tcp_pcb_t *pcb = (tcp_pcb_t *)s->tcp_pcb;
        pcb->local_port = s->local_port;
        pcb->state = TCP_LISTEN;
        return 0;
    }
    return -1;
}

vfs_node_t* socket_accept(vfs_node_t *node, struct sockaddr *addr, uint32_t *addrlen) {
    // Şimdilik sadece bloklayan beklemeyi simüle edelim
    socket_t *s = (socket_t *)node->ptr;
    if (s->type == SOCK_STREAM) {
        tcp_pcb_t *pcb = (tcp_pcb_t *)s->tcp_pcb;
        while (pcb->state != TCP_ESTABLISHED) {
            s->waiting_task = get_current_task();
            s->waiting_task->state = TASK_SLEEP;
            schedule();
        }
        // Aslında yeni bir socket donulmeli ama basite indirgedik
        return node; 
    }
    return NULL;
}

int socket_sendto(vfs_node_t *node, const void *message, uint32_t length, int flags, const struct sockaddr *dest_addr, uint32_t dest_len) {
    socket_t *s = (socket_t *)node->ptr;
    if (s->type == SOCK_DGRAM) {
        uint32_t ip_addr;
        uint16_t port;
        
        if (dest_addr) {
            struct sockaddr_in *sin = (struct sockaddr_in *)dest_addr;
            ip_addr = sin->sin_addr.s_addr;
            port = ntohs(sin->sin_port);
        } else if (s->remote_addr != 0) {
            ip_addr = s->remote_addr;
            port = s->remote_port;
        } else {
            return -1;
        }
        
        unsigned char ip[4];
        memcpy(ip, &ip_addr, 4);
        net_send_udp(ip, port, s->local_port, (void *)message, length);
        return length;
    } else if (s->type == SOCK_STREAM) {
        tcp_pcb_t *pcb = (tcp_pcb_t *)s->tcp_pcb;
        if (pcb && pcb->state == TCP_ESTABLISHED) {
            tcp_send_segment(pcb, TCP_ACK | TCP_PSH, (uint8_t *)message, (uint16_t)length);
            return length;
        }
    }
    return -1;
}

int socket_recvfrom(vfs_node_t *node, void *buffer, uint32_t length, int flags, struct sockaddr *address, uint32_t *address_len) {
    socket_t *s = (socket_t *)node->ptr;
    
    while (s->buffer_len == 0) {
        s->waiting_task = get_current_task();
        s->waiting_task->state = TASK_SLEEP;
        schedule(); 
    }

    if (s->type == SOCK_DGRAM) {
        // Başlığı oku
        socket_packet_t packet;
        uint8_t *p = (uint8_t*)&packet;
        for (int i = 0; i < sizeof(socket_packet_t); i++) {
            p[i] = s->buffer[s->buffer_tail];
            s->buffer_tail = (s->buffer_tail + 1) % SOCKET_BUFFER_SIZE;
        }
        
        // Veriyi oku
        uint32_t to_read = (length < packet.data_len) ? length : packet.data_len;
        for (uint32_t i = 0; i < packet.data_len; i++) {
            if (i < to_read) {
                ((uint8_t *)buffer)[i] = s->buffer[s->buffer_tail];
            }
            s->buffer_tail = (s->buffer_tail + 1) % SOCKET_BUFFER_SIZE;
        }
        
        // Adres bilgisini doldur
        if (address && address_len && *address_len >= sizeof(struct sockaddr_in)) {
            struct sockaddr_in *sin = (struct sockaddr_in *)address;
            sin->sin_family = AF_INET;
            sin->sin_port = htons(packet.src_port);
            sin->sin_addr.s_addr = packet.src_ip;
            *address_len = sizeof(struct sockaddr_in);
        }
        
        unsigned int flags_irq = irq_save();
        s->buffer_len -= (sizeof(socket_packet_t) + packet.data_len);
        irq_restore(flags_irq);

        return to_read;
    } else {
        // TCP: Stream (Header yok)
        uint32_t to_read = (length < s->buffer_len) ? length : s->buffer_len;
        for (uint32_t i = 0; i < to_read; i++) {
            ((uint8_t *)buffer)[i] = s->buffer[s->buffer_tail];
            s->buffer_tail = (s->buffer_tail + 1) % SOCKET_BUFFER_SIZE;
        }
        
        unsigned int flags_irq = irq_save();
        s->buffer_len -= to_read;
        irq_restore(flags_irq);
        
        return to_read;
    }
}

void socket_dispatch_udp(uint16_t dest_port, uint8_t *data, uint16_t len, uint8_t *src_ip, uint16_t src_port) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].in_use && sockets[i].local_port == dest_port) {
            uint32_t src_ip_val = *(uint32_t*)src_ip;
            socket_packet_t packet;
            packet.src_ip = src_ip_val;
            packet.src_port = src_port;
            packet.data_len = len;
            
            // Tamponda yer var mı? (Tüm paket sığmalı)
            if (sockets[i].buffer_len + sizeof(socket_packet_t) + len <= SOCKET_BUFFER_SIZE) {
                // Başlığı kopyala
                uint8_t *p = (uint8_t*)&packet;
                for (int j = 0; j < sizeof(socket_packet_t); j++) {
                    sockets[i].buffer[sockets[i].buffer_head] = p[j];
                    sockets[i].buffer_head = (sockets[i].buffer_head + 1) % SOCKET_BUFFER_SIZE;
                }
                // Veriyi kopyala
                for (int j = 0; j < len; j++) {
                    sockets[i].buffer[sockets[i].buffer_head] = data[j];
                    sockets[i].buffer_head = (sockets[i].buffer_head + 1) % SOCKET_BUFFER_SIZE;
                }
                
                unsigned int flags_irq = irq_save();
                sockets[i].buffer_len += (sizeof(socket_packet_t) + len);
                irq_restore(flags_irq);
                
                if (sockets[i].waiting_task) {
                    sockets[i].waiting_task->state = TASK_READY;
                    sockets[i].waiting_task = NULL;
                }
            }
            break;
        }
    }
}
