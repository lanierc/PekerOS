; boot/boot.s
MAGIC    equ 0x1BADB002
FLAGS    equ 0x03 ; Bit 0: Align, Bit 1: Memory Info
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text
align 16
global gdt_ptr_asm
gdt_ptr_asm:
    dw 0    ; limit
    dd 0    ; base

align 16
global idt_ptr_asm
idt_ptr_asm:
    dw 0    ; limit
    dd 0    ; base

global _start
_start:
    cli
    ; ebx contains multiboot info pointer (physical)
    ; eax contains multiboot magic number
    mov ebp, eax ; Save magic number in ebp

    ; Set up physical stack temporarily
    mov esp, (stack_top - 0xC0000000)

    ; Populate 128 boot page tables (512MB)
    mov edi, (boot_page_table1 - 0xC0000000)
    mov esi, 0
    mov ecx, 131072 ; 128 tables * 1024 entries = 512MB
.fill_table:
    mov edx, esi
    or edx, 3 ; Present + R/W
    mov [edi], edx
    add edi, 4
    add esi, 4096
    loop .fill_table

    ; Set up boot_page_directory
    mov edi, (boot_page_directory - 0xC0000000)
    
    ; Loop to map 128 tables to both identity and higher-half
    mov eax, (boot_page_table1 - 0xC0000000)
    or eax, 3
    mov ecx, 128
    mov esi, 0
.map_tables:
    ; Identity map (0, 1, 2...)
    mov [edi + esi * 4], eax
    ; Higher-half map (768, 769, 770...)
    mov [edi + (768 + esi) * 4], eax
    
    add eax, 4096 ; Next table
    inc esi
    loop .map_tables

    ; Enable paging
    mov eax, (boot_page_directory - 0xC0000000)
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Jump to higher half
    lea eax, [_higher_half]
    jmp eax

_higher_half:
    ; Unmap identity mapping (optional but safe)
    mov dword [boot_page_directory], 0
    invlpg [0]

    ; Now use virtual stack
    mov esp, stack_top

    ; Convert multiboot info pointer to virtual address
    add ebx, 0xC0000000

    push ebx            ; arg2: multiboot info pointer
    push ebp            ; arg1: multiboot magic number
    extern kernel_main
    call kernel_main
    add esp, 8
.hang:
    cli
    hlt
    jmp .hang

global gdt_flush
gdt_flush:
    lgdt [gdt_ptr_asm]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush
.flush:
    ret

global idt_flush
idt_flush:
    lidt [idt_ptr_asm]
    ret

global tss_flush
tss_flush:
    mov ax, 0x28      ; TSS selector (index 5, RPL 0)
    ltr ax
    ret

global switch_to_user_mode
switch_to_user_mode:
    cli
    mov ax, 0x23      ; User Data Selector (0x20 | 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
    push 0x23         ; SS
    push eax          ; ESP
    pushfd            ; EFLAGS
    
    pop eax
    or eax, 0x200     ; Enable Interrupts
    push eax

    push 0x1B         ; CS (0x18 | 3)
    push .user_entry
    iret

.user_entry:
    extern user_mode_test
    call user_mode_test
    jmp $

extern isr_handler
isr_common_stub:
    pusha
    xor eax, eax
    mov ax, ds
    push eax
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

extern irq_handler
irq_common_stub:
    pusha
    xor eax, eax
    mov ax, ds
    push eax
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    mov esp, eax    ; Return value is the new stack pointer!

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

global int80_handler
int80_handler:
    push byte 0
    push dword 128
    pusha
    xor eax, eax
    mov ax, ds
    push eax
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    extern syscall_handler
    call syscall_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

global isr0
isr0:
    push byte 0
    push byte 0
    jmp isr_common_stub

global irq0
irq0:
    push byte 0
    push byte 32
    jmp irq_common_stub

global irq1
irq1:
    push byte 0
    push byte 33
    jmp irq_common_stub

global irq2
irq2:
    push byte 0
    push byte 34
    jmp irq_common_stub

global irq3
irq3:
    push byte 0
    push byte 35
    jmp irq_common_stub

global irq4
irq4:
    push byte 0
    push byte 36
    jmp irq_common_stub

global irq5
irq5:
    push byte 0
    push byte 37
    jmp irq_common_stub

global irq6
irq6:
    push byte 0
    push byte 38
    jmp irq_common_stub

global irq7
irq7:
    push byte 0
    push byte 39
    jmp irq_common_stub

global irq8
irq8:
    push byte 0
    push byte 40
    jmp irq_common_stub

global irq9
irq9:
    push byte 0
    push byte 41
    jmp irq_common_stub

global irq10
irq10:
    push byte 0
    push byte 42
    jmp irq_common_stub

global irq11
irq11:
    push byte 0
    push byte 43
    jmp irq_common_stub

global irq12
irq12:
    push byte 0
    push byte 44
    jmp irq_common_stub

global irq13
irq13:
    push byte 0
    push byte 45
    jmp irq_common_stub

global irq14
irq14:
    push byte 0
    push byte 46
    jmp irq_common_stub

global irq15
irq15:
    push byte 0
    push byte 47
    jmp irq_common_stub

global isr14
isr14:
    ; CPU automatically pushes an error code for exception 14 (Page Fault)
    push byte 14
    jmp isr_common_stub

global load_page_directory
load_page_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax
    mov esp, ebp
    pop ebp
    ret

global enable_paging
enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    mov esp, ebp
    pop ebp
    ret

section .bss
align 4096
boot_page_directory:
    resb 4096
boot_page_table1:
    resb 4096 * 128 ; 128 tables for 512MB

align 16
stack_bottom:
    resb 16384
global stack_top
stack_top: