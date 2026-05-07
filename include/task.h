#ifndef TASK_H
#define TASK_H

#include "common.h"

// Görev Durumları
#define TASK_READY   0
#define TASK_RUNNING 1
#define TASK_SLEEP   2
#define TASK_DEAD    3

// Görev Kontrol Bloğu (TCB)
typedef struct task {
    int id;                // Process ID (PID)
    char name[32];         // Görev adı
    unsigned int esp;      // Mevcut Stack Pointer
    unsigned int kstack_top; // Kernel Stack Tepesi (TSS için)
    unsigned int* page_dir;// Sayfa dizini (şimdilik çekirdek dizini)
    int state;             // Görev durumu
    struct vfs_node* fd_table[16]; // Dosya tanımlayıcı tablosu
    unsigned int fd_offset[16];    // Dosya okuma/yazma konumları
    unsigned int heap_start;       // sbrk başlangıcı
    unsigned int heap_end;         // sbrk sonu
    struct task* next;     // Sonraki görev (Basit bir bağlı liste)
} task_t;

// Fonksiyonlar
void init_multitasking();
task_t* create_task(char* name, void (*entry_point)(), int is_user);
void schedule();
task_t* get_current_task();
unsigned int schedule_internal(unsigned int current_esp);
void task_exit();

#endif
