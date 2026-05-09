// Sayı Tahmin Oyunu (Userland)

// Syscall tanımları
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

void print(const char *msg) {
    syscall1(1, (int)msg);
}

int get_ticks() {
    return syscall0(0);
}

char get_char() {
    char c = 0;
    while (c == 0) {
        c = (char)syscall0(3);
    }
    return c;
}

void main() {
    char *msg = "Sayi Tahmin Oyununa Hosgeldiniz!\n";
    print("\n--- Sayi Tahmin Oyunu ---\n");
    print("1 ile 9 arasinda bir sayi tuttum.\n");
    print("Tahmininizi girin (Cikmak icin 'q'):\n");

    // Basit bir rastgele sayı üretimi (uptime tabanlı)
    int secret = (get_ticks() % 9) + 1;
    
    while(1) {
        print("> ");
        char c = get_char();
        
        // Ekranda göster
        char buf[2];
        buf[0] = c;
        buf[1] = '\0';
        print(buf);
        print("\n");

        if (c == 'q' || c == 'Q') {
            print("Cikiliyor...\n");
            break;
        }

        if (c >= '1' && c <= '9') {
            int guess = c - '0';
            if (guess == secret) {
                print("Tebrikler! Dogru tahmin ettiniz.\n");
                break;
            } else if (guess < secret) {
                print("Daha buyuk!\n");
            } else {
                print("Daha kucuk!\n");
            }
        } else {
            print("Lutfen 1-9 arasi bir sayi girin.\n");
        }
    }

    // Çıkış
    syscall0(2);
    while(1) {}
}
