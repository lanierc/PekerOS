#include "socket.h"

// Userland print helper
extern int _write(int file, char *ptr, int len);
void print(char *s) {
    int len = 0;
    while(s[len]) len++;
    _write(1, s, len);
}

void main() {
    print("UDP Echo Server baslatiliyor (Port 8888)...\n");
    
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        print("Hata: Soket olusturulamadi!\n");
        return;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    server_addr.sin_addr.s_addr = 0; // INADDR_ANY
    
    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        print("Hata: Bind basarisiz!\n");
        return;
    }
    
    print("Paket bekleniyor...\n");
    
    char buffer[128];
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    
    while(1) {
        int n = recvfrom(fd, buffer, 127, 0, &client_addr, &addr_len);
        if (n > 0) {
            buffer[n] = '\0';
            print("Gelen Mesaj: ");
            print(buffer);
            print("\n");
            
            // Cevap gönder (Paketin geldiği yere geri gönderiyoruz)
            print("Cevaplaniyor...\n");
            sendto(fd, buffer, n, 0, &client_addr, sizeof(client_addr));
        }
    }
}
