ENTRY(_start)
SECTIONS
{
    /* Uygulama 0x08048000 adresine yüklenecek şekilde ayarlandı */
    . = 0x08048000;

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}
