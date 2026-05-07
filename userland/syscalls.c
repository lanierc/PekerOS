// LibC Syscall Glue Code (Stubs)
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

// ------------------------------------
// POSIX Standart Fonksiyonları
// ------------------------------------

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
