// Minimal Userland Program
void _start() {
    const char *msg = ">>> Merhaba! Bu bir ELF dosyasidir. <<<\n";
    
    // SYS_WRITE (1) sistem çağrısını manuel tetikleyelim
    // eax = 1, ebx = mesaj adresi
    asm volatile("int $0x80" : : "a"(1), "b"(msg));

    // Programın bitmemesi için sonsuz döngü (Şimdilik SYS_EXIT tam hazır değilse)
    while(1) {
        asm volatile("nop");
    }
}
