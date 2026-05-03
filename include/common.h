#ifndef COMMON_H
#define COMMON_H

// Her yerde kullanılacak olan register yapısı
struct registers {
    unsigned int ds;                  // push ds (elle itilen)
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha
    unsigned int int_no, err_code;    // Bizim ittiğimiz (irqX)
    unsigned int eip, cs, eflags;     // İşlemcinin ittiği
};

// Port işlemleri (kernel.c içinde tanımlayacağız)
void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);
void outw(unsigned short port, unsigned short val);
unsigned short inw(unsigned short port);
void outl(unsigned short port, unsigned int val);
unsigned int inl(unsigned short port);

// Terminal fonksiyonları
void put_str(const char* str);
void put_char(char c);
void put_hex(unsigned int n);
void put_int(int n);

#endif