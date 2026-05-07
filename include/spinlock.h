#ifndef SPINLOCK_H
#define SPINLOCK_H

/**
 * FerkanOS Spinlock & Interrupt Control
 * 
 * Şu an için tek çekirdekli sistemlerde yarış durumlarını önlemek için 
 * kesmeleri (interrupts) kapatıp açma yöntemini kullanıyoruz.
 */

// Kesme durumunu saklayıp kesmeleri kapatır
static inline unsigned int irq_save() {
    unsigned int flags;
    asm volatile (
        "pushfl\n\t"
        "popl %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

// Önceki kesme durumunu geri yükler
static inline void irq_restore(unsigned int flags) {
    asm volatile (
        "pushl %0\n\t"
        "popfl"
        :
        : "g"(flags)
        : "memory", "cc"
    );
}

// Basit spinlock yapısı (Şimdilik sadece kesmeleri kontrol eder)
typedef struct {
    volatile int locked;
} spinlock_t;

#define SPINLOCK_INIT {0}

static inline void spin_lock(spinlock_t *lock) {
    asm volatile("cli");
    lock->locked = 1;
}

static inline void spin_unlock(spinlock_t *lock) {
    lock->locked = 0;
    asm volatile("sti");
}

#endif
