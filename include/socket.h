#ifndef SOCKET_H
#define SOCKET_H

#include "common.h"
#include "vfs.h"
#include "task.h"

#define AF_INET     2
#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

#pragma pack(push, 1)
struct sockaddr {
    uint16_t sa_family;
    char     sa_data[14];
};

struct in_addr {
    uint32_t s_addr;
};

struct sockaddr_in {
    uint16_t       sin_family;
    uint16_t       sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};
#pragma pack(pop)

#define SOCKET_BUFFER_SIZE 4096

typedef struct socket_packet {
    uint32_t src_ip;
    uint16_t src_port;
    uint16_t data_len;
    // Data follows immediately
} socket_packet_t;

typedef struct socket {
    int domain;
    int type;
    int protocol;
    
    uint16_t local_port;
    uint32_t local_addr;
    
    uint16_t remote_port;
    uint32_t remote_addr;
    
    uint8_t *buffer;
    uint32_t buffer_head;
    uint32_t buffer_tail;
    uint32_t buffer_len;
    
    task_t *waiting_task;
    void *tcp_pcb;
    int in_use;
} socket_t;

vfs_node_t* socket_create(int domain, int type, int protocol);
int socket_bind(vfs_node_t *node, const struct sockaddr *addr, uint32_t addrlen);
int socket_connect(vfs_node_t *node, const struct sockaddr *addr, uint32_t addrlen);
int socket_listen(vfs_node_t *node, int backlog);
vfs_node_t* socket_accept(vfs_node_t *node, struct sockaddr *addr, uint32_t *addrlen);
int socket_sendto(vfs_node_t *node, const void *message, uint32_t length, int flags, const struct sockaddr *dest_addr, uint32_t dest_len);
int socket_recvfrom(vfs_node_t *node, void *buffer, uint32_t length, int flags, struct sockaddr *address, uint32_t *address_len);

#endif
