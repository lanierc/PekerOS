#ifndef SYSCALL_H
#define SYSCALL_H

#include "common.h"

#define SYS_UPTIME 0
#define SYS_WRITE  1
#define SYS_EXIT   2

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

#endif
