// Mini-Vi (Pilo) Metin Editörü

int _write(int file, char *ptr, int len);
int _read(int file, char *ptr, int len);
int _open(const char *name, int flags, int mode);
int _close(int file);
void _exit(int status);
void _clear();

static inline int syscall0(int num) {
    int a; asm volatile("int $0x80" : "=a" (a) : "0" (num)); return a;
}

char get_char() {
    char c = 0; while (c == 0) { c = (char)syscall0(3); } return c;
}

int strlen(const char *s) {
    int i = 0; while(s[i]) i++; return i;
}
void print(char *s) { _write(1, s, strlen(s)); }

void _start() {
    _clear();
    print("--- Mini-Vi (Pilo) Editoru ---\n");
    print("Kaydetmek ve cikmak icin 'ESC' tusuna basin.\n");
    print("Dosya: test.txt\n");
    print("--------------------------------------------------------------------------------");
    
    // O_CREAT (1) ile aç
    int fd = _open("test.txt", 1, 0);
    if (fd < 0) {
        print("Hata: Dosya acilamadi!\n");
        _exit(1);
    }
    
    char buf[2048];
    int len = _read(fd, buf, 2047);
    if (len < 0) len = 0;
    buf[len] = '\0';
    
    print(buf);
    
    while (len < 2047) {
        char c = get_char();
        if (c == 27) { // ESC key (ASCII 27)
            break;
        }
        
        // Ekrana bas
        char s[2] = {c, 0};
        print(s);
        
        // Backspace desteği
        if (c == '\b') {
            if (len > 0) len--;
        } else {
            buf[len++] = c;
        }
    }
    
    // Dosyayı kaydet. 
    // Mevcut fd'yi kapatıp, baştan yazmak üzere O_CREAT ile tekrar açıyoruz.
    _close(fd);
    
    fd = _open("test.txt", 1, 0);
    _write(fd, buf, len);
    _close(fd);
    
    print("\n[OK] Dosya kaydedildi. Cikiliyor...\n");
    _exit(0);
}
