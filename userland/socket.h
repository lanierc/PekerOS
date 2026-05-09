#ifndef USER_SOCKET_H
#define USER_SOCKET_H

typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define NULL ((void*)0)

#define AF_INET     2
#define SOCK_STREAM 1
#define SOCK_DGRAM  2

int write(int fd, const void *buf, int len);
int send(int fd, const void *buf, int len, int flags);
int recv(int fd, void *buf, int len, int flags);

struct in_addr {
    uint32_t s_addr;
};

struct sockaddr_in {
    uint16_t       sin_family;
    uint16_t       sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

struct sockaddr {
    uint16_t sa_family;
    char     sa_data[14];
};

int socket(int domain, int type, int protocol);
int bind(int fd, const void *addr, int addrlen);
int connect(int fd, const void *addr, int addrlen);
int listen(int fd, int backlog);
int accept(int fd, void *addr, int *addrlen);
int sendto(int fd, const void *buf, int len, int flags, const void *addr, int addrlen);
int recvfrom(int fd, void *buf, int len, int flags, void *addr, int *addrlen);

// Helper for IP address
uint32_t inet_addr(const char *cp);
uint16_t htons(uint16_t hostshort);

#endif
