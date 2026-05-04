#include "elf.h"
#include "vfs.h"
#include "kheap.h"
#include "paging.h"
#include "pmm.h"

int elf_load(vfs_node_t *base, const char *filename) {
    vfs_node_t *node = vfs_get_node_by_path(base, filename);
    
    if (!node) {
        put_str("[ELF] Dosya bulunamadi: ");
        put_str(filename);
        put_str("\n");
        return -1;
    }

    char *buffer = (char *)kmalloc(node->length);
    int read_len = vfs_read(node, 0, node->length, (unsigned char *)buffer);
    
    if (read_len <= 0) {
        put_str("[ELF] Dosya okunamadi: ");
        put_str(filename);
        put_str("\n");
        kfree(node);
        kfree(buffer);
        return -1;
    }

    Elf32_Ehdr *header = (Elf32_Ehdr *)buffer;

    // 2. Magic Number Kontrolü
    if (header->e_ident[0] != ELFMAG0 || header->e_ident[1] != ELFMAG1 ||
        header->e_ident[2] != ELFMAG2 || header->e_ident[3] != ELFMAG3) {
        put_str("[ELF] Gecersiz ELF magic number!\n");
        kfree(buffer);
        return -1;
    }

    // 3. Program Header'ları Gez
    Elf32_Phdr *ph = (Elf32_Phdr *)(buffer + header->e_phoff);
    for (int i = 0; i < header->e_phnum; i++) {
        if (ph[i].p_type == PT_LOAD) {
            // Segmenti belleğe haritala
            // p_vaddr: Sanal Adres, p_memsz: Bellekteki boyutu, p_filesz: Dosyadaki boyutu
            
            unsigned int start_virt = ph[i].p_vaddr;
            unsigned int mem_size = ph[i].p_memsz;
            unsigned int file_size = ph[i].p_filesz;
            unsigned int offset = ph[i].p_offset;

            // Fiziksel frame'leri ayır ve haritala
            // Not: paging_map_memory fiziksel adres bekler. 
            // Burada yeni frame'ler alıp map etmeliyiz.
            int num_pages = (mem_size + 4095) / 4096;
            for (int j = 0; j < num_pages; j++) {
                unsigned int frame = alloc_frame();
                paging_map_memory(frame, start_virt + (j * 4096), 4096);
            }

            // Veriyi kopyala
            memcpy((void *)start_virt, buffer + offset, file_size);

            // BSS (Sıfırlanması gereken alan) varsa sıfırla
            if (mem_size > file_size) {
                memset((void *)(start_virt + file_size), 0, mem_size - file_size);
            }
        }
    }

    unsigned int entry = header->e_entry;
    kfree(buffer);
    kfree(node);
    
    put_str("[ELF] Yuklendi: ");
    put_str(filename);
    put_str(" -> Entry: 0x");
    put_hex(entry);
    put_str("\n");

    return (int)entry;
}
