#include "syscall.h"
#include "timer.h"
#include "vfs.h"
#include "task.h"
#include "paging.h"
#include "pmm.h"

static void* syscall_table[10];

extern unsigned char keyboard_get();
extern void task_exit();

void sys_getchar(struct registers *regs) {
    regs->eax = keyboard_get();
}

void sys_exit(struct registers *regs) {
    task_exit();
}

void sys_open(struct registers *regs) {
    char *path = (char *)regs->ebx;
    int flags = regs->ecx;
    
    vfs_node_t *node = vfs_get_node_by_path(vfs_root, path);
    if (!node) {
        if (flags & 1) { // 1 = O_CREAT
            extern int pafs_write(const char *name, const char *data, int len);
            pafs_write(path, "", 0);
            node = vfs_get_node_by_path(vfs_root, path);
            if (!node) {
                regs->eax = -1;
                return;
            }
        } else {
            regs->eax = -1;
            return;
        }
    }
    task_t *current = get_current_task();
    for (int i = 0; i < 16; i++) {
        if (current->fd_table[i] == 0) {
            current->fd_table[i] = node;
            current->fd_offset[i] = 0;
            regs->eax = i;
            return;
        }
    }
    vfs_close(node);
    regs->eax = -1;
}

void sys_read(struct registers *regs) {
    int fd = regs->ebx;
    char *buf = (char *)regs->ecx;
    int size = regs->edx;
    
    task_t *current = get_current_task();
    if (fd < 0 || fd >= 16 || current->fd_table[fd] == 0) {
        regs->eax = -1;
        return;
    }
    int read_bytes = vfs_read(current->fd_table[fd], current->fd_offset[fd], size, (unsigned char*)buf);
    if (read_bytes > 0) {
        current->fd_offset[fd] += read_bytes;
    }
    regs->eax = read_bytes;
}

void sys_write_fd(struct registers *regs) {
    int fd = regs->ebx;
    char *buf = (char *)regs->ecx;
    int size = regs->edx;
    
    if (fd == 1 || fd == 2) {
        for (int i = 0; i < size; i++) put_char(buf[i]);
        regs->eax = size;
        return;
    }
    
    task_t *current = get_current_task();
    if (fd < 0 || fd >= 16 || current->fd_table[fd] == 0) {
        regs->eax = -1;
        return;
    }
    int written = vfs_write(current->fd_table[fd], current->fd_offset[fd], size, (unsigned char*)buf);
    if (written > 0) {
        current->fd_offset[fd] += written;
    }
    regs->eax = written;
}

void sys_close(struct registers *regs) {
    int fd = regs->ebx;
    task_t *current = get_current_task();
    if (fd < 0 || fd >= 16 || current->fd_table[fd] == 0) {
        regs->eax = -1;
        return;
    }
    vfs_close(current->fd_table[fd]);
    current->fd_table[fd] = 0;
    current->fd_offset[fd] = 0;
    regs->eax = 0;
}

void sys_sbrk(struct registers *regs) {
    int increment = regs->ebx;
    task_t *current = get_current_task();
    
    unsigned int old_end = current->heap_end;
    if (increment == 0) {
        regs->eax = old_end;
        return;
    }
    
    unsigned int new_end = old_end + increment;
    
    unsigned int old_page = (old_end + 4095) / 4096;
    unsigned int new_page = (new_end + 4095) / 4096;
    
    for (unsigned int i = old_page; i < new_page; i++) {
        unsigned int frame = alloc_frame();
        paging_map_memory(frame, i * 4096, 4096);
    }
    
    current->heap_end = new_end;
    regs->eax = old_end;
}

extern void clear_scr();
void sys_clear(struct registers *regs) {
    clear_scr();
}

void sys_uptime(struct registers *regs) {
    regs->eax = timer_get_ticks();
}

void sys_write(struct registers *regs) {
    char* str = (char*)regs->ebx;
    put_str(str);
}

void syscall_handler(struct registers *regs) {
    // EAX içerisinde syscall numarası var
    if (regs->eax >= 10) return;

    void *handler = syscall_table[regs->eax];
    if (!handler) return;

    // Handler fonksiyonunu çağır
    void (*handler_func)(struct registers *) = handler;
    handler_func(regs);
}

void init_syscalls() {
    syscall_table[SYS_UPTIME] = sys_uptime;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_EXIT] = sys_exit;
    syscall_table[SYS_GETCHAR] = sys_getchar;
    syscall_table[SYS_OPEN] = sys_open;
    syscall_table[SYS_READ] = sys_read;
    syscall_table[SYS_WRITE_FD] = sys_write_fd;
    syscall_table[SYS_CLOSE] = sys_close;
    syscall_table[SYS_SBRK] = sys_sbrk;
    syscall_table[SYS_CLEAR] = sys_clear;
    
    put_str("[OK] Sistem cagrilari (Syscalls) baslatildi.\n");
}
