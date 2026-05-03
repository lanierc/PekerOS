ENTRY(_start)
SECTIONS
{
    /* VMA (Sanal Adres): 3GB + 1MB */
    . = 0xC0100000;
    __kernel_start = .;
    
    __text_start = .;
    /* LMA (Fiziksel Yükleme Adresi): VMA - 3GB */
    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000) { 
        __multiboot_start = .;
        *(.multiboot) 
        *(.text) 
    }
    __text_end = .;
    
    __rodata_start = .;
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000) { *(.rodata) }
    __rodata_end = .;
    
    __data_start = .;
    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000) { *(.data) }
    __data_end = .;
    
    __bss_start = .;
    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000) { 
        *(COMMON) 
        *(.bss) 
    }
    __bss_end = .;
    
    __kernel_end = .;
}

__phys_multiboot_start = __multiboot_start - 0xC0000000;
__phys_text_start = __text_start - 0xC0000000;
__phys_bss_end = __bss_end - 0xC0000000;
__phys_start = _start - 0xC0000000;