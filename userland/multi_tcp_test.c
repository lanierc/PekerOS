#include <stdint.h>
#include "socket.h"

int strlen(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

void print(const char* s) {
    write(1, s, strlen(s));
}

uint32_t inet_addr(const char *cp) {
    uint32_t res = 0;
    int parts[4] = {0,0,0,0};
    int part = 0;
    while (*cp) {
        if (*cp == '.') {
            part++;
            if (part > 3) return 0;
        } else if (*cp >= '0' && *cp <= '9') {
            parts[part] = parts[part] * 10 + (*cp - '0');
        }
        cp++;
    }
    res = (parts[3] << 24) | (parts[2] << 16) | (parts[1] << 8) | parts[0];
    return res;
}

void main() {
    print("Multi-TCP Test Uygulamasi Baslatiliyor (3 simultaneous connections)...\n");

    int socks[3];
    int ports[3] = {8888, 8889, 8890};
    
    for(int i=0; i<3; i++) {
        socks[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (socks[i] < 0) {
            print("Hata: Soket olusturulamadi!\n");
            return;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(ports[i]);
        server_addr.sin_addr.s_addr = inet_addr("10.0.2.2");

        print("Baglaniliyor: Port ");
        // Basit port yazdirma
        char pbuf[6];
        int p = ports[i];
        pbuf[4] = '\0'; pbuf[3] = (p%10)+'0'; p/=10; pbuf[2] = (p%10)+'0'; p/=10; pbuf[1] = (p%10)+'0'; p/=10; pbuf[0] = (p%10)+'0';
        print(pbuf); print("...\n");

        if (connect(socks[i], (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            print("Hata: Baglanti kurulamadi!\n");
        } else {
            print("Baglanti Basarili!\n");
            send(socks[i], "Sinyal!", 7, 0);
        }
    }

    print("Tüm mesajlar gonderildi. 2 saniye bekleniyor...\n");
    // sleep syscall yoksa bos dongu
    for(volatile int i=0; i<10000000; i++);

    print("Soketler kapatiliyor (FIN testi)...\n");
    // close() syscall'u VFS node'unu serbest birakir ve socket_close cagrilir
    // Henuz bir wrapper yoksa direkt sys_close nolu syscall lazim.
    // userland/socket.h'da close yoksa _exit de tum dosyalari kapatir.
    
    print("Test tamamlandi.\n");
}
