#include "shell.h"
#include "common.h"
#include "timer.h"
#include "pmm.h"
#include "pafs.h"

// Metin fonksiyonları (kernel.c içinde tanımladık)
extern int strcmp(const char *s1, const char *s2);
extern int strlen(const char *s);
extern void clear_scr();

#define BUFFER_SIZE 256
static char input_buffer[BUFFER_SIZE];
static int buffer_idx = 0;

#define HISTORY_MAX 10
static char history_buffer[HISTORY_MAX][BUFFER_SIZE];
static int history_count = 0;
static int history_nav_idx = -1;

// Basit bir strncmp
static int strncmp(const char *s1, const char *s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

// Basit atoi: string -> int
static int atoi(const char *s) {
    int result = 0;
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    return result;
}

void print_prompt() {
    put_str("fash> ");
}

void init_shell() {
    buffer_idx = 0;
    history_count = 0;
    history_nav_idx = 0;
    for(int i=0; i<BUFFER_SIZE; i++) input_buffer[i] = 0;
    print_prompt();
}

void execute_command(char* cmd) {
    if (strlen(cmd) == 0) return;

    if (strcmp(cmd, "help") == 0) {
        put_str("Komutlar:\n");
        put_str("  help     - Bu yardim mesajini goster\n");
        put_str("  clear    - Ekrani temizle\n");
        put_str("  version  - Sistem surumunu goster\n");
        put_str("  uptime   - Sistemin calisme suresini goster\n");
        put_str("  ticks    - Zamanlayici tick sayisini goster\n");
        put_str("  mem      - Bellek istatistiklerini goster\n");
        put_str("  sleep N  - N milisaniye bekle (ornek: sleep 1000)\n");
        put_str("  echo ... - Mesaji ekrana yaz\n");
        put_str("  ls       - PAFS kok dizinini listele\n");
        put_str("  touch f  - PAFS'ta 'f' adinda dosya olustur\n");
        put_str("  write f t- 'f' dosyasina 't' metnini yaz\n");
        put_str("  cat f    - 'f' dosyasini oku\n");
        put_str("  reboot   - Sistemi yeniden baslat\n");
    } 
    else if (strcmp(cmd, "clear") == 0) {
        clear_scr();
        extern unsigned int terminal_row, terminal_col;
        terminal_row = 0;
        terminal_col = 0;
    } 
    else if (strcmp(cmd, "version") == 0) {
        put_str("PekerOS v0.2 - Shell: fash v0.1\n");
    }
    else if (strcmp(cmd, "uptime") == 0) {
        unsigned int total_secs = timer_get_seconds();
        unsigned int mins = total_secs / 60;
        unsigned int secs = total_secs % 60;
        put_str("Sistem calisme suresi: ");
        put_int(mins);
        put_str(" dakika, ");
        put_int(secs);
        put_str(" saniye (");
        put_int(timer_get_ticks());
        put_str(" tick)\n");
    }
    else if (strcmp(cmd, "ticks") == 0) {
        put_str("Toplam tick: ");
        put_int(timer_get_ticks());
        put_str("\n");
    }
    else if (strcmp(cmd, "mem") == 0) {
        put_str("--- Bellek Istatistikleri ---\n");
        put_str("Toplam Bellek: ");
        put_int(pmm_total_memory_kb() / 1024);
        put_str(" MB (");
        put_int(pmm_total_memory_kb());
        put_str(" KB)\n");
        
        put_str("Toplam Frame (4KB): ");
        put_int(pmm_total_frames());
        put_str("\n");
        
        put_str("Kullanilan Frame: ");
        put_int(pmm_used_frames());
        put_str(" (");
        put_int((pmm_used_frames() * 4) / 1024);
        put_str(" MB)\n");
        
        put_str("Bos Frame: ");
        put_int(pmm_free_frames());
        put_str(" (");
        put_int((pmm_free_frames() * 4) / 1024);
        put_str(" MB)\n");
    }
    else if (strncmp(cmd, "sleep ", 6) == 0) {
        int ms = atoi(cmd + 6);
        if (ms <= 0) {
            put_str("Kullanim: sleep <milisaniye> (ornek: sleep 1000)\n");
        } else {
            put_str("Bekleniyor: ");
            put_int(ms);
            put_str(" ms...\n");
            sleep(ms);
            put_str("Tamamlandi.\n");
        }
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        put_str(cmd + 5);
        put_str("\n");
    }
    else if (strcmp(cmd, "echo") == 0) {
        put_str("\n");
    }
    else if (strcmp(cmd, "ls") == 0) {
        pafs_list_dir();
    }
    else if (strncmp(cmd, "touch ", 6) == 0) {
        char *filename = cmd + 6;
        if (strlen(filename) == 0) {
            put_str("Kullanim: touch <dosya_adi>\n");
        } else {
            int ino = pafs_create(filename, 0);
            if (ino != -1) {
                put_str("Dosya olusturuldu: "); put_str(filename); put_str("\n");
            } else {
                put_str("Hata: Dosya olusturulamadi (Disk dolu olabilir).\n");
            }
        }
    }
    else if (strncmp(cmd, "write ", 6) == 0) {
        char *filename = cmd + 6;
        char *text = 0;
        for (int i = 0; filename[i] != '\0'; i++) {
            if (filename[i] == ' ') {
                filename[i] = '\0';
                text = &filename[i+1];
                break;
            }
        }
        if (text == 0 || strlen(filename) == 0) {
            put_str("Kullanim: write <dosya_adi> <metin>\n");
        } else {
            int written = pafs_write(filename, text, strlen(text));
            if (written != -1) {
                put_str("Yazildi ("); put_int(written); put_str(" byte).\n");
            } else {
                put_str("Hata: Yazma basarisiz (Dosya bulunamadi veya disk dolu).\n");
            }
        }
    }
    else if (strncmp(cmd, "cat ", 4) == 0) {
        char *filename = cmd + 4;
        if (strlen(filename) == 0) {
            put_str("Kullanim: cat <dosya_adi>\n");
        } else {
            char buf[513];
            int read_len = pafs_read(filename, buf, 512);
            if (read_len != -1) {
                put_str("--- "); put_str(filename); put_str(" ---\n");
                put_str(buf);
                put_str("\n--------------------\n");
            } else {
                put_str("Hata: Dosya bulunamadi veya okunamadi.\n");
            }
        }
    }
    else if (strcmp(cmd, "reboot") == 0) {
        put_str("Sistem yeniden baslatiliyor...\n");
        // Klavye kontrolcüsü üzerinden reset (8042 port 0x64)
        outb(0x64, 0xFE);
    }
    else {
        put_str("Hata: Bilinmeyen komut '");
        put_str(cmd);
        put_str("'. 'help' yazarak komutlari gorebilirsiniz.\n");
    }
}

void shell_input(char c) {
    if (c == '\n') {
        put_char('\n');
        input_buffer[buffer_idx] = '\0';
        
        // Komut geçmişine kaydet
        if (buffer_idx > 0) {
            if (history_count == 0 || strcmp(history_buffer[history_count - 1], input_buffer) != 0) {
                if (history_count < HISTORY_MAX) {
                    for(int i = 0; i <= buffer_idx; i++) history_buffer[history_count][i] = input_buffer[i];
                    history_count++;
                } else {
                    for (int i = 0; i < HISTORY_MAX - 1; i++) {
                        for(int j = 0; j < BUFFER_SIZE; j++) history_buffer[i][j] = history_buffer[i+1][j];
                    }
                    for(int i = 0; i <= buffer_idx; i++) history_buffer[HISTORY_MAX - 1][i] = input_buffer[i];
                }
            }
        }
        
        execute_command(input_buffer);
        
        // Tamponu sıfırla
        buffer_idx = 0;
        for(int i=0; i<BUFFER_SIZE; i++) input_buffer[i] = 0;
        history_nav_idx = history_count;
        
        print_prompt();
    } 
    else if (c == '\x11') { // Yukarı Ok (Önceki komut)
        if (history_count > 0 && history_nav_idx > 0) {
            history_nav_idx--;
            // Ekrandaki mevcut yazıyı sil
            while (buffer_idx > 0) {
                put_char('\b');
                buffer_idx--;
            }
            // Geçmişteki komutu yaz
            int i = 0;
            while (history_buffer[history_nav_idx][i]) {
                input_buffer[i] = history_buffer[history_nav_idx][i];
                put_char(input_buffer[i]);
                i++;
            }
            buffer_idx = i;
            input_buffer[buffer_idx] = '\0';
        }
    }
    else if (c == '\x12') { // Aşağı Ok (Sonraki komut)
        if (history_nav_idx < history_count) {
            history_nav_idx++;
            // Ekrandaki mevcut yazıyı sil
            while (buffer_idx > 0) {
                put_char('\b');
                buffer_idx--;
            }
            if (history_nav_idx == history_count) {
                buffer_idx = 0;
                input_buffer[0] = '\0';
            } else {
                int i = 0;
                while (history_buffer[history_nav_idx][i]) {
                    input_buffer[i] = history_buffer[history_nav_idx][i];
                    put_char(input_buffer[i]);
                    i++;
                }
                buffer_idx = i;
                input_buffer[buffer_idx] = '\0';
            }
        }
    }
    else if (c == '\b') {
        if (buffer_idx > 0) {
            buffer_idx--;
            input_buffer[buffer_idx] = 0;
            put_char('\b');
        }
    } 
    else {
        if (buffer_idx < BUFFER_SIZE - 1) {
            input_buffer[buffer_idx++] = c;
            put_char(c);
        }
    }
}
