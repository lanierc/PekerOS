#ifndef SYSCALL_H
#define SYSCALL_H

#include "common.h"

#define SYS_UPTIME 0
#define SYS_WRITE  1
#define SYS_EXIT   2
#define SYS_GETCHAR 3
#define SYS_OPEN   4
#define SYS_READ   5
#define SYS_WRITE_FD 6
#define SYS_CLOSE  7
#define SYS_SBRK   8
#define SYS_CLEAR  9
#define SYS_SOCKET 10
#define SYS_BIND   11
#define SYS_SENDTO 12
#define SYS_RECVFROM 13
#define SYS_CONNECT 14
#define SYS_LISTEN 15
#define SYS_ACCEPT 16

void init_syscalls();

// Kullanıcı tarafı için syscall sarmalayıcıları (Assembly inline)
static inline int syscall0(int num) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (num));
    return a;
}

static inline int syscall1(int num, int p1) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" (p1));
    return a;
}

static inline int syscall2(int num, int p1, int p2) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" (p1), "c" (p2));
    return a;
}

static inline int syscall3(int num, int p1, int p2, int p3) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" (p1), "c" (p2), "d" (p3));
    return a;
}

static inline int syscall4(int num, int p1, int p2, int p3, int p4) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" (p1), "c" (p2), "d" (p3), "S" (p4));
    return a;
}

static inline int syscall5(int num, int p1, int p2, int p3, int p4, int p5) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" (p1), "c" (p2), "d" (p3), "S" (p4), "D" (p5));
    return a;
}

#endif
