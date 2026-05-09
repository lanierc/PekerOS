#define NULL ((void*)0)
typedef unsigned short uint16_t;
uint16_t htons(uint16_t n) {
    return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8);
}
uint16_t ntohs(uint16_t n) {
    return htons(n);
}
extern void main();
void _exit(int status);

__attribute__((section(".text.entry"), used))
void _start() {
    main();
    _exit(0);
}

// Bu dosya ileride Newlib/Musl derlenirken 'libos.a' olarak baglanacaktir.

typedef unsigned int size_t;

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

// ------------------------------------
// POSIX Standart Fonksiyonları
// ------------------------------------

int socket(int domain, int type, int protocol) {
    return syscall3(10, domain, type, protocol);
}

int bind(int fd, const void *addr, int addrlen) {
    return syscall3(11, fd, (int)addr, addrlen);
}

int connect(int fd, const void *addr, int addrlen) {
    return syscall3(14, fd, (int)addr, addrlen);
}

int listen(int fd, int backlog) {
    return syscall2(15, fd, backlog);
}

int accept(int fd, void *addr, int *addrlen) {
    return syscall3(16, fd, (int)addr, (int)addrlen);
}

int sendto(int fd, const void *buf, int len, int flags, const void *addr, int addrlen) {
    return syscall5(12, fd, (int)buf, len, (int)addr, addrlen);
}

int recvfrom(int fd, void *buf, int len, int flags, void *addr, int *addrlen) {
    return syscall5(13, fd, (int)buf, len, (int)addr, (int)addrlen);
}

int send(int fd, const void *buf, int len, int flags) {
    return sendto(fd, buf, len, flags, NULL, 0);
}

int recv(int fd, void *buf, int len, int flags) {
    return recvfrom(fd, buf, len, flags, NULL, NULL);
}

int write(int fd, const void *buf, int len) {
    return syscall3(6, fd, (int)buf, len);
}

void _exit(int status) {
    syscall0(2); // SYS_EXIT
    while(1);
}

int _open(const char *name, int flags, int mode) {
    return syscall1(4, (int)name); // SYS_OPEN
}

int _read(int file, char *ptr, int len) {
    return syscall3(5, file, (int)ptr, len); // SYS_READ
}

int _write(int file, char *ptr, int len) {
    return syscall3(6, file, (int)ptr, len); // SYS_WRITE_FD
}

int _close(int file) {
    return syscall1(7, file); // SYS_CLOSE
}

void *_sbrk(int incr) {
    return (void *)syscall1(8, incr); // SYS_SBRK
}

void _clear() {
    syscall0(9); // SYS_CLEAR
}

// Şimdilik boş olan fonksiyonlar (Newlib derlenmesi için gereklidir)
int _fstat(int file, void *st) {
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

int _kill(int pid, int sig) {
    return -1;
}

int _getpid(void) {
    return 1;
}
