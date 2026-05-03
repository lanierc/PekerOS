#include "syscall.h"
#include "timer.h"

static void* syscall_table[3];

void sys_uptime(struct registers *regs) {
    regs->eax = timer_get_ticks();
}

void sys_write(struct registers *regs) {
    char* str = (char*)regs->ebx;
    put_str(str);
}

void syscall_handler(struct registers *regs) {
    // EAX içerisinde syscall numarası var
    if (regs->eax >= 3) return;

    void *handler = syscall_table[regs->eax];
    if (!handler) return;

    // Handler fonksiyonunu çağır
    void (*handler_func)(struct registers *) = handler;
    handler_func(regs);
}

void init_syscalls() {
    syscall_table[SYS_UPTIME] = sys_uptime;
    syscall_table[SYS_WRITE] = sys_write;
    
    put_str("[OK] Sistem cagrilari (Syscalls) baslatildi.\n");
}
